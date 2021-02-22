#!/usr/bin/awk -f
BEGIN {
    FS="=";
    file=ARGV[1];
    xl=ENVIRON["XL"];
    base=file;
    sub("[.]xl", "", base);
    csource=file;
    sub("[.]xl", ".c", csource);
    subdir=file;
    sub("/[^/]*$", "", subdir);
}
/\/\/ +[A-Z]+=/ {
    name=substr($1, match($1, "[A-Z]"));
    value=substr($0, index($0, "=") + 1);
    gsub("%x", xl, value);
    gsub("%f", file, value);
    gsub("%b", base, value);
    gsub("%c", csource, value);
    gsub("%d", subdir, value);
    print base "." name "=" value;
}
