#lang rosette

(require (except-in rackunit fail)
         rackunit/text-ui
         rosette/lib/roseunit
         serval/llvm
         serval/lib/unittest
         serval/lib/core)

(require "generated/racket/test/allocation.globals.rkt"
         "generated/racket/test/allocation.map.rkt")

(require "generated/racket/test/allocation.ll.rkt")

(define PAGE_SIZE #x1000)
(define NUMBER_OF_PAGES #x10000)
(define PAGE_FREE 0)
(define PAGE_FRAME 1)
(define PAGE_L1_ENTRY 2)
(define PAGE_L2_ENTRY 3)
(define PAGE_L3_ENTRY 4)
(define PAGE_L4_ENTRY 5)
(define PAGE_NUMBER_TYPE_SIZE 64)
(define PAGE_TYPE_SIZE 32)
(define PID_TYPE_SIZE 16)

(define (inv)
  (define-symbolic i (bitvector 64))
  (define b0 (symbol->block 'all_frames))
  (define b1 (symbol->block 'frames_metadata))
  (forall (list i) (=> (bvult i (bv NUMBER_OF_PAGES PAGE_NUMBER_TYPE_SIZE))
		       (=> (equal? (mblock-iload b1 (list i 'type)) (bv PAGE_FREE PAGE_TYPE_SIZE))
			   (equal? (mblock-iload b1 (list i 'refcount)) (bv 0 64))))))

(define (check-allocation-spec)
  (parameterize ([current-machine (make-machine symbols globals)])
    (define-symbolic frame_number (bitvector PAGE_NUMBER_TYPE_SIZE))
    (define-symbolic type (bitvector PAGE_TYPE_SIZE))
    (define-symbolic permissions (bitvector 32))
    (define pre (inv))
    (define asserted
      (with-asserts-only (@allocate_frame frame_number type permissions)))
    (define post (inv))
    ; no UB triggered
    (print (asserts))
    (check-equal? (asserts) null)
    (for-each (lambda (x) (check-unsat? (verify (assert x)))) asserted)
    ; check if the invariant holds
    (check-unsat? (verify (assert (implies pre post))))))

(define allocation-tests
  (test-suite+
   "Tests for allocation.c"

   (test-case+ "check-allocation-spec" (check-allocation-spec))))

(module+ test
  (time (run-tests allocation-tests)))
