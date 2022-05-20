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

(define PAGE_NUMBER_TYPE_SIZE 64)
(define PAGE_TYPE_SIZE 32)
(define PID_TYPE_SIZE 16)
(define PAGE_SIZE #x1000)
(define NUMBER_OF_PAGES #x10000)
(define PAGE_FREE (bv 0 PAGE_TYPE_SIZE))
(define PAGE_FRAME (bv 1 PAGE_TYPE_SIZE))
(define PAGE_L1_ENTRY (bv 2 PAGE_TYPE_SIZE))
(define PAGE_L2_ENTRY (bv 3 PAGE_TYPE_SIZE))
(define PAGE_L3_ENTRY (bv 4 PAGE_TYPE_SIZE))
(define PAGE_L4_ENTRY (bv 5 PAGE_TYPE_SIZE))

(define (inv)
  (define-symbolic i (bitvector 64))
  (define b1 (symbol->block 'frames_metadata))
  (forall (list i) (=> (bvult i (bv NUMBER_OF_PAGES PAGE_NUMBER_TYPE_SIZE))
		       ;(print (mcell-size b1 (list i 'type))))
		       (&&
			 (=> (equal? (mblock-iload b1 (list i 'type)) PAGE_L4_ENTRY)
			     (&&
			       ; L4 entries can't be referenced by anything
			       (equal? (mblock-iload b1 (list i 'refcount)) (bv 0 32))))
			 ; Free pages can't be referenced by anything
			 (=> (equal? (mblock-iload b1 (list i 'type)) PAGE_FREE)
			     (equal? (mblock-iload b1 (list i 'refcount)) (bv 0 32)))))))

(define (check-allocation-spec-allocate)
  (parameterize ([current-machine (make-machine symbols globals)])
    ; Verifying that the allocate_frame function does not violate the invariant
    (define-symbolic frame_number (bitvector PAGE_NUMBER_TYPE_SIZE))
    (define-symbolic type (bitvector PAGE_TYPE_SIZE))
    (define-symbolic permissions (bitvector 32))
    (define pre (inv))
    (define asserted
      (with-asserts-only (@allocate_frame frame_number type permissions)))
    (define post (inv))
    ; no UB triggered
    (for-each (lambda (x) (check-unsat? (verify (assert x)))) (asserts))
    (for-each (lambda (x) (check-unsat? (verify (assert x)))) asserted)
    ; check if the invariant holds
    (check-unsat? (verify (assert (implies pre post))))))

(define (check-allocation-spec-free)
  (parameterize ([current-machine (make-machine symbols globals)])
    ; Verifying that the free_frame function does not violate the invariant
    (define-symbolic frame_number (bitvector PAGE_NUMBER_TYPE_SIZE))
    (define pre (inv))
    (define asserted
      (with-asserts-only (@free_frame frame_number)))
    (define post (inv))
    ; no UB triggered
    (for-each (lambda (x) (check-unsat? (verify (assert x)))) (asserts))
    (for-each (lambda (x) (check-unsat? (verify (assert x)))) asserted)
    ; check if the invariant holds
    (check-unsat? (verify (assert (implies pre post))))))

; Because of the long loop in init_frames, this function takes forever
; For a small number of pages, this function works
;(define (check-allocation-spec-init)
;  (parameterize ([current-machine (make-machine symbols globals)])
;    ; Verifying that the init_frames function leaves the array in a state that verifies the invariant
;    (define pre #t)
;    (define asserted
;      (with-asserts-only (@init_frames)))
;    (define post (inv))
;    ; no UB triggered
;    (for-each (lambda (x) (check-unsat? (verify (assert x)))) (asserts))
;    (for-each (lambda (x) (check-unsat? (verify (assert x)))) asserted)
;    ; check if the invariant holds
;    (check-unsat? (verify (assert (implies pre post))))))

(define allocation-tests
  (test-suite+
   "Tests for allocation.c"

   (test-case+ "check-allocation-spec-allocate" (check-allocation-spec-allocate))
   (test-case+ "check-allocation-spec-free" (check-allocation-spec-free))
   ;(test-case+ "check-allocation-spec-init" (check-allocation-spec-init))
   ))

(module+ test
  (time (run-tests allocation-tests)))
