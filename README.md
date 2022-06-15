# Servaled paging

C library handling page allocation and page mapping mechanisms. Intended to be
proved correct by Serval.


## How to prove with Serval

From the root directory of this repository, run the following commands:

```
docker run -it --name serval unsat/serval-tools:artifact
```

This will open a shell in the Serval container. Open another terminal and go to
the root directory of this repository again, then run from there:

```
docker cp $(pwd) serval:/serval/racket/test
```

Then, from inside the Serval container :

```
cp /usr/riscv64-linux-gnu/include/gnu/stubs-lp64d.h /usr/riscv64-linux-gnu/include/gnu/stubs-lp64.h
cd /serval/racket/test/
cp servaled-paging/{src/all.c,include/all.h,specs/all.rkt} .
make -C ../../ raco-test
```

There should be 2 errors: they come from a missing library required by some of
Serval's own tests. They can be ignored. Any other error should come from the
allocation spec and implementation.

To make the verification faster, you can remove some of the verifications done
on other Serval test files by replacing 
in `/serval/racket/racket.mk` :

```
RACO_TESTS      += $(wildcard racket/test/*.rkt)
```

by

```
override RACO_TESTS = racket/test/all.rkt
```

With only the tests of the allocation/paging library remaining, all tests
should pass. Otherwise send me an email.


## Directory structure

The headers are in `include/` and the source files are in `src/`.
