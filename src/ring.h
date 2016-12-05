// ****************************************************************************
//  ring.h                                                        ELFE project
// ****************************************************************************
//
//   File Description:
//
//    A circular buffer with multiple writers, generally single reader.
//
//    This implementation is supposed to work in multi-CPU configurations
//    without using locks, only using the primitives in "atomic.h"
//
//
//
//
//
// ****************************************************************************
// This document is released under the GNU General Public License, with the
// following clarification and exception.
//
// Linking this library statically or dynamically with other modules is making
// a combined work based on this library. Thus, the terms and conditions of the
// GNU General Public License cover the whole combination.
//
// As a special exception, the copyright holders of this library give you
// permission to link this library with independent modules to produce an
// executable, regardless of the license terms of these independent modules,
// and to copy and distribute the resulting executable under terms of your
// choice, provided that you also meet, for each linked independent module,
// the terms and conditions of the license of that module. An independent
// module is a module which is not derived from or based on this library.
// If you modify this library, you may extend this exception to your version
// of the library, but you are not obliged to do so. If you do not wish to
// do so, delete this exception statement from your version.
//
// See http://www.gnu.org/copyleft/gpl.html and Matthew 25:22 for details
//  (C) 1992-2010 Christophe de Dinechin <christophe@taodyne.com>
//  (C) 2010 Taodyne SAS
// ****************************************************************************
/*
 * Implementation of a thread-safe, non-blocking ring buffer.
 * This ring buffer can work in multi-processor scenarios or multi-threaded
 * systems, and can be used to store records of any type. It can also safely
 * manage sequences of consecutive records. A typical use case is strings
 * in a console print buffer, which have variable length.
 *
 * How this works:
 *   Each buffer is represented by
 *   - an array A of N items (for performance, N should be a power of 2),
 *   - a reader index R,
 *   - a writer index W
 *   - a commit index C
 *   - an overflow counter O
 *
 *   The core invariants of the structure are (ignoring integer overflow)
 *   1.  R <= C <= W
 *   2.  overflowed = W - R >= N
 *
 *   Reading entries from the buffer consist in the following steps:
 *   1. If the buffer overflowed, "catch up":
 *   1a. Set R to W-N+1
 *   2b. Increase O to record the overflow
 *   2. There is readable data iff R < C. If so:
 *   2a. Read A[R % N] (possibly blocking)
 *   2b. Atomically increase R
 *   
 *   Writing E entries in the buffer consists in the following steps:
 *   1. Atomically increase W, fetching the old W (fetch_and_add)
 *   2. Copy the entries in A[oldW % N] (possibly blocking)
 *   3. Wait until C == oldW, and Atomically set C to W (possibly blocking)
 *
 * Step 2 can block if the reader has not caught up yet.
 * Step 3 can block if another writer has still not updated C
 *
 * Important notes:
 *   The code as written is safe for use with C++ objects. Do not attempt
 *   to replace data copies with memcpy(), as it would most likely not
 *   improve speed (because of the "modulo" to perform) and would break C++.
 *
 *   In theory, if you use the buffer long enough, all indexes will ultimately
 *   wrap around. This is why all comparisons are done with something like
 *       (int) (writer - reader) >= size 
 *   instead of a solution that would fail when writer wraps around
 *       writer >= reader + size
 *   It is therefore assumed that you will never create a buffer of a
 *   size larger than 2^31 on a 32-bit machine. Probably OK.
 *   
 */

#ifndef RING_H
#define RING_H

#include "atomic.h"
#include <iterator>


ELFE_BEGIN

template <typename Type, unsigned Size>
struct Ring
// ----------------------------------------------------------------------------
//    Ring buffer declaration
// ----------------------------------------------------------------------------
{
public:
    enum { size = Size };
    typedef Type type;

public:
    Ring(kstring name): name(name), reader(), writer(), commit(), overflow()
    {}

public:
    // Attributes and methods for the ring buffer
    kstring  Name()  { return name; }
    unsigned Readable();
    unsigned Writable();

    // All reads return number of elements read
    // All writes return location of first element read in circular buffer
    
    // Reading and writing a single element by value
    bool     Read(type &data);
    unsigned Write(const type &data);

    // Non-blocking read and write of a sequence of elements
    template <typename Iterator>
    unsigned Read(Iterator begin, Iterator end);
    template <typename Iterator>
    unsigned Write(Iterator begin, Iterator end);
    
    // Blocking read and write
    template <typename Iterator, typename Block, typename Overflow>
    unsigned Read(Iterator begin, Iterator end,
                  Block block, Overflow overflow);
    template <typename Iterator, typename WriteBlock, typename CommitBlock>
    unsigned Write(Iterator begin, Iterator end,
                   WriteBlock write_block, CommitBlock commit_block);

public:
    // Interface for block handling
    unsigned        Reader()        { return reader; }
    unsigned        Writer()        { return writer; }
    unsigned        Commit()        { return commit; }
    unsigned        Overflow()      { return commit; }
    
    // Skip n elements from reading
    void Skip(unsigned skip)        { reader += skip; }

    // Record read overflow
    void Overflow(unsigned skip)    { overflow += skip; }

    // Commit by skipping
    void Commit(unsigned skip)      { commit += skip; }
    
private:
    kstring          name;         // Name for debugging purpose
    Atomic<unsigned> reader;       // Reader index
    Atomic<unsigned> writer;       // Writer index
    Atomic<unsigned> commit;       // Last commited write
    Atomic<unsigned> overflow;     // Overflowed writes
    Type             data[Size];   // Circular buffer itself
};



// ============================================================================
// 
//    Some default representations for Block and Overflow
// 
// ============================================================================

template <class Ring>
struct AbortReadIfBlocking
// ----------------------------------------------------------------------------
//    If a read is blocking, abort it (return 0)
// ----------------------------------------------------------------------------
{
    AbortReadIfBlocking(Ring *ring): ring(ring) {}
    bool operator()(unsigned from, unsigned to) { return false; }
    Ring *ring;
};


template <class Ring>
struct ProceedWithWriteIfBlocking
// ----------------------------------------------------------------------------
//    If a read is blocking, abort it (return 0)
// ----------------------------------------------------------------------------
{
    ProceedWithWriteIfBlocking(Ring *ring): ring(ring) {}
    bool operator()(unsigned from, unsigned to) { return true; }
    Ring *ring;
};


template<class Ring>
struct SkipAndRecordOverflow
// ----------------------------------------------------------------------------
//    Default behavior on read overflow is to skip forward and record overflow
// ----------------------------------------------------------------------------
{
    SkipAndRecordOverflow(Ring *ring): ring(ring) {}
    bool operator()(unsigned reader, unsigned writer)
    {
        unsigned minR = writer - ring->size + 1;
        unsigned skip = minR - reader;
        ring->Skip(skip);
        ring->Overflow(skip);
        return true;
    }
    Ring *ring;
};


template <class Ring>
struct CommitBySkipping
// ----------------------------------------------------------------------------
//   Default behavior at end of write is to commit by adding an offset
// ----------------------------------------------------------------------------
{
    CommitBySkipping(Ring *ring): ring(ring) {}
    bool operator()(unsigned oldW, unsigned lastW)
    {
        unsigned count = lastW - oldW;
        ring->Commit(count);
        return false;
    }
    Ring *ring;
};



// ============================================================================
// 
//   Default inline template implementations
// 
// ============================================================================

template <typename Type, unsigned Size> inline
unsigned Ring<Type, Size>::Readable()
// ----------------------------------------------------------------------------
//  The number of readable elements
// ----------------------------------------------------------------------------
{
    return commit - reader;
}


template <typename Type, unsigned Size> inline
unsigned Ring<Type, Size>::Writable()
// ----------------------------------------------------------------------------
//   The number of writable elements
// ----------------------------------------------------------------------------
{
    unsigned written = writer - reader;
    unsigned writable = size - written - 1;
    if (written >= size - 1)
        writable = 0;           // Check if overflowed
    return writable;
}



template <typename Type, unsigned Size> inline
bool Ring<Type, Size>::Read(type &data)
// ----------------------------------------------------------------------------
//    Read a single element from the ring
// ----------------------------------------------------------------------------
{
    return Read(&data, &data + 1) > 0;
}


template <typename Type, unsigned Size> inline
unsigned Ring<Type, Size>::Write(const Type &data)
// ----------------------------------------------------------------------------
//    Write a single element in the ring
// ----------------------------------------------------------------------------
{
    return Write(&data, &data + 1);
}


template <typename Type, unsigned Size>
template <typename Iterator, typename Block, typename Over>
unsigned Ring<Type, Size>::Read(Iterator begin,
                                Iterator end,
                                Block    block,
                                Over     overflow)
// ----------------------------------------------------------------------------
//  Read up to 'count' elements from 'rb' into 'ptr', return itertor for last
// ----------------------------------------------------------------------------
{
    Iterator current = begin;
    while (current != end)
    {
        unsigned reader = this->reader; // Keep local copy for consistency
        unsigned writer = this->writer;

        // Check if we read beyond what may be currently written
        if (writer - reader >= size)
            if (overflow(reader, writer))
                break;

        // Check if we have something to read
        unsigned commit = this->commit;
        if ((int) (reader - commit) >= 0)
            if (!block(reader, commit))
                break;

        // Read data from current position
        *current = this->data[reader % size];
        if (reader == this->reader++)
            current++;
    }
    return current - begin;
}


template <typename Type, unsigned Size>
template <typename Iterator>
unsigned Ring<Type, Size>::Read(Iterator begin, Iterator end)
// ----------------------------------------------------------------------------
//   Non-blocking Read variant
// ----------------------------------------------------------------------------
{
    typedef Ring<Type, Size> R;
    return Read(begin, end,
                AbortReadIfBlocking<R>(this), SkipAndRecordOverflow<R>(this));
}


template <typename Type, unsigned Size>
template <typename Iterator, typename WriteBlock, typename CommitBlock>
unsigned Ring<Type, Size>::Write(Iterator    begin,
                                 Iterator    end,
                                 WriteBlock  write_block,
                                 CommitBlock commit_block)
// ----------------------------------------------------------------------------
//   Write 'count' elements from 'ptr' into 'rb', return first index
// ----------------------------------------------------------------------------
{
    unsigned count = std::distance(begin, end);
    unsigned oldW = this->writer.Add(count);
    unsigned lastW = oldW + count;
    unsigned reader = this->reader;
    unsigned lastSafeW = reader + size - 1;

    // Optimize writes that can't possibly overwrite reader
    if ((int) (lastSafeW - lastW) > 0)
        lastSafeW = lastW;

    // Write everything that is not at risk to overwrite reader
    unsigned w = oldW;
    Iterator current = begin;
    while ((int) (w - lastSafeW) < 0)
        this->data[w++ % size] = *current++;

    // Slower write for things that may require us to block
    while ((int) (w - lastW) < 0)
    {
        // If we are overwriting reader data, block or abort copy
        if (w - this->reader >= size - 1 && !write_block(w, lastW))
            break;
        this->data[w++ % size] = *current++;
    }

    // We are done, commit buffer change, but only if commit is oldW
    // Otherwise, some other guy before us is still copying data,
    // so we need to wait.
    while (!this->commit.SetQ(oldW, lastW))
        if (!commit_block(oldW, lastW))
            break;

    return oldW;
}


template <typename Type, unsigned Size>
template <typename Iterator>
unsigned Ring<Type, Size>::Write(Iterator begin, Iterator end)
// ----------------------------------------------------------------------------
//   Non-blocking Read variant
// ----------------------------------------------------------------------------
{
    typedef Ring<Type, Size> R;
    return Write(begin, end,
                 ProceedWithWriteIfBlocking<R>(this),CommitBySkipping<R>(this));
}

ELFE_END

#endif // RING_H
