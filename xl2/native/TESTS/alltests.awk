#!/usr/bin/awk -f
BEGIN {
    FS="=";
    file=ARGV[1];
    xl=ENVIRON["XL"];
    base=file;
    sub("[.]xl", "", base);
}
/\/\/ [A-Z]+=/ {
    name=substr($1, 4);
    value=$2;
    gsub("%x", xl, value);
    gsub("%f", file, value);
    gsub("%b", base, value);
    print name "='" value "'";
}

