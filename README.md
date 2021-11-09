csapp labs文件备份

```
.
├── Dockerfile  // 新的环境
├── Dockerfile.old  // 老的环境
├── README.md
├── lab // tar包源文件
│   ├── bomb.tar
│   ├── cachelab-handout.tar
│   ├── malloclab-handout.tar
│   ├── shlab-handout.tar
│   └── target1.tar
└── solution
    ├── bomb
    ├── cachelab-handout
    ├── malloclab-handout
    ├── shlab-handout
    └── target1
```

希望自己能不弃坑

---

malloclab w/ segregated list

throughput score in range 28-40
```
Results for mm malloc:
trace  valid  util     ops      secs  Kops
 0       yes   92%    5694  0.001056  5390
 1       yes   94%    5848  0.001591  3677
 2       yes   96%    6648  0.000830  8011
 3       yes   98%    5380  0.000602  8937
 4       yes   50%   14400  0.001305 11031
 5       yes   88%    4800  0.001590  3018
 6       yes   85%    4800  0.001249  3842
 7       yes   55%   12000  0.000854 14047
 8       yes   51%   24000  0.000935 25666
 9       yes   28%   14401  0.142202   101
10       yes   30%   14401  0.004528  3180
Total          70%  112372  0.156743   717

Perf index = 42 (util) + 40 (thru) = 82/100
```