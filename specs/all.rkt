#lang rosette

(require (except-in rackunit fail)
         rackunit/text-ui
         rosette/lib/roseunit
         serval/llvm
         serval/lib/unittest
         serval/lib/core)

(require "generated/racket/test/all.globals.rkt"
         "generated/racket/test/all.map.rkt")

(require "generated/racket/test/all.ll.rkt")


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
(define PAGE_ACCESS_READ   (bv #x1 32))
(define PAGE_ACCESS_WRITE  (bv #x2 32))
(define PAGE_ACCESS_EXEC   (bv #x4 32))
(define PAGE_ACCESS_SHARED (bv #x8 32))

(define (inv)
  (define-symbolic i (bitvector 64))
  (define-symbolic j (bitvector 64))
  (define-symbolic k (bitvector 64))
  (define fm (symbol->block 'frames_metadata))
  (define fs (symbol->block 'all_frames))
  (forall (list i) (=> (bvult i (bv NUMBER_OF_PAGES PAGE_NUMBER_TYPE_SIZE))
		       (&&
			 ; L4 entries can't be referenced by anything
			 (=> (equal? (mblock-iload fm (list i 'type)) PAGE_L4_ENTRY)
			     (&&
			       (equal? (mblock-iload fm (list i 'refcount)) (bv 0 32))))
			 ; Free pages can't be referenced by anything
			 (=> (equal? (mblock-iload fm (list i 'type)) PAGE_FREE)
			     (equal? (mblock-iload fm (list i 'refcount)) (bv 0 32)))
			 ; frames' addresses are not null, and they are unique
			 (not (equal? (mblock-iload fm (list i 'address)) (bv 0 64)))
			 (forall (list j) (=> (bvult j (bv NUMBER_OF_PAGES PAGE_NUMBER_TYPE_SIZE))
					      (not (equal? (mblock-iload fm (list i 'address)) (mblock-iload fm (list j 'address))))))
			 ;(=> (equal? (mblock-iload fm (list i 'address)) (mblock-iload fm (list j 'address)))
			 ;    (equal? i j))
			 ; L4 pages must point to L3 pages
			 ; frames_metadata[i].type == L4 -> &all_frames[j] == all_frames[i][k] -> fm[j].type == L3
			 (=> (equal? (mblock-iload fm (list i 'type)) PAGE_L4_ENTRY)
			     (forall (list j) (forall (list k)
				(=> (bvult j (bv NUMBER_OF_PAGES PAGE_NUMBER_TYPE_SIZE))
				   (=> (bvult k (bv 512 PAGE_NUMBER_TYPE_SIZE))
				     (=> (not (equal? (mblock-iload fs (list i 'content k)) (bv 0 64))) 
				       (=> (equal? (mblock-iload fm (list j 'address)) (mblock-iload fs (list i 'content k)))
					   (equal? (mblock-iload fm (list j 'type)) PAGE_L3_ENTRY))))))))
			 ; L3 pages must point to L2 pages
			 ; frames_metadata[i].type == L3 -> &all_frames[j] == all_frames[i][k] -> fm[j].type == L2
			 (=> (equal? (mblock-iload fm (list i 'type)) PAGE_L3_ENTRY)
			     (forall (list j) (forall (list k)
				(=> (bvult j (bv NUMBER_OF_PAGES PAGE_NUMBER_TYPE_SIZE))
				   (=> (bvult k (bv 512 PAGE_NUMBER_TYPE_SIZE))
				     (=> (not (equal? (mblock-iload fs (list i 'content k)) (bv 0 64))) 
				       (=> (equal? (mblock-iload fm (list j 'address)) (mblock-iload fs (list i 'content k)))
					   (equal? (mblock-iload fm (list j 'type)) PAGE_L2_ENTRY))))))))
			 ; L2 pages must point to L1 pages
			 ; frames_metadata[i].type == L2 -> &all_frames[j] == all_frames[i][k] -> fm[j].type == L1
			 (=> (equal? (mblock-iload fm (list i 'type)) PAGE_L2_ENTRY)
			     (forall (list j) (forall (list k)
				(=> (bvult j (bv NUMBER_OF_PAGES PAGE_NUMBER_TYPE_SIZE))
				   (=> (bvult k (bv 512 PAGE_NUMBER_TYPE_SIZE))
				     (=> (not (equal? (mblock-iload fs (list i 'content k)) (bv 0 64))) 
				       (=> (equal? (mblock-iload fm (list j 'address)) (mblock-iload fs (list i 'content k)))
					   (equal? (mblock-iload fm (list j 'type)) PAGE_L1_ENTRY))))))))
			 ; L1 pages must point to frames
			 ; frames_metadata[i].type == L1 -> &all_frames[j] == all_frames[i][k] -> fm[j].type == FRAME
			 (=> (equal? (mblock-iload fm (list i 'type)) PAGE_L1_ENTRY)
			     (forall (list j) (forall (list k)
				(=> (bvult j (bv NUMBER_OF_PAGES PAGE_NUMBER_TYPE_SIZE))
				   (=> (bvult k (bv 512 PAGE_NUMBER_TYPE_SIZE))
				     (=> (not (equal? (mblock-iload fs (list i 'content k)) (bv 0 64))) 
				       (=> (equal? (mblock-iload fm (list j 'address)) (mblock-iload fs (list i 'content k)))
					   (equal? (mblock-iload fm (list j 'type)) PAGE_FRAME))))))))
			 ; shared tables only point to shared entries
			 ; frames_metadata[i].type == L{1,2,3,4} && frames_metadata[i].permissions & SHARED -> &all_frames[j] == all_frames[i][k] -> fm[j].permissions & SHARED
			 (=> (&& (|| (equal? (mblock-iload fm (list i 'type)) PAGE_L4_ENTRY)
				     (equal? (mblock-iload fm (list i 'type)) PAGE_L3_ENTRY)
				     (equal? (mblock-iload fm (list i 'type)) PAGE_L2_ENTRY)
				     (equal? (mblock-iload fm (list i 'type)) PAGE_L1_ENTRY))
				 (equal? (bvand (mblock-iload fm (list i 'permissions)) PAGE_ACCESS_SHARED) PAGE_ACCESS_SHARED))
			     (forall (list j) (forall (list k)
				(=> (bvult j (bv NUMBER_OF_PAGES PAGE_NUMBER_TYPE_SIZE))
				   (=> (bvult k (bv 512 PAGE_NUMBER_TYPE_SIZE))
				     (=> (not (equal? (mblock-iload fs (list i 'content k)) (bv 0 64))) 
				       (=> (equal? (mblock-iload fm (list j 'address)) (mblock-iload fs (list i 'content k)))
					   (equal? (bvand (mblock-iload fm (list i 'permissions)) PAGE_ACCESS_SHARED) PAGE_ACCESS_SHARED))))))))
			 ; exclusive tables can only point to exclusive tables/frames that they own
			 ; frames_metadata[i].type == L{1,2,3,4} && frames_metadata[i].permissions ^ SHARED -> &all_frames[j] == all_frames[i][k] -> fm[j].permissions ^ SHARED -> fm[j].owner == fm[i].owner
			 (=> (&& (|| (equal? (mblock-iload fm (list i 'type)) PAGE_L4_ENTRY)
				     (equal? (mblock-iload fm (list i 'type)) PAGE_L3_ENTRY)
				     (equal? (mblock-iload fm (list i 'type)) PAGE_L2_ENTRY)
				     (equal? (mblock-iload fm (list i 'type)) PAGE_L1_ENTRY))
				 (equal? (bvand (mblock-iload fm (list i 'permissions)) PAGE_ACCESS_SHARED) (bv 0 32)))
			     (forall (list j) (forall (list k)
				(=> (bvult j (bv NUMBER_OF_PAGES PAGE_NUMBER_TYPE_SIZE))
				   (=> (bvult k (bv 512 PAGE_NUMBER_TYPE_SIZE))
				     (=> (not (equal? (mblock-iload fs (list i 'content k)) (bv 0 64))) 
				       (=> (equal? (mblock-iload fm (list j 'address)) (mblock-iload fs (list i 'content k)))
					   (=> (equal? (bvand (mblock-iload fm (list j 'permissions)) PAGE_ACCESS_SHARED) (bv 0 32))
					       (equal? (mblock-iload fm (list i 'owner)) (mblock-iload fm (list j 'owner)))))))))))
			 ))))

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
(define (check-allocation-spec-init)
  (parameterize ([current-machine (make-machine symbols globals)])
    ; Verifying that the init_frames function leaves the array in a state that verifies the invariant
    (define pre #t)
    (define asserted
      (with-asserts-only (@init_frames)))
    (define post (inv))
    ; no UB triggered
    (for-each (lambda (x) (check-unsat? (verify (assert x)))) (asserts))
    (for-each (lambda (x) (check-unsat? (verify (assert x)))) asserted)
    ; check if the invariant holds
    (check-unsat? (verify (assert (implies pre post))))))

(define (check-paging-spec-unmap)
  (parameterize ([current-machine (make-machine symbols globals)])
    ; Verifying that the free_frame function does not violate the invariant
    (define-symbolic pte (bitvector PAGE_NUMBER_TYPE_SIZE))
    (define-symbolic offset (bitvector 16))
    (define-symbolic pte_entry (bitvector PAGE_NUMBER_TYPE_SIZE))
    (define pre (inv))
    (define asserted
      (with-asserts-only (@unmap_page_table_entry pte offset pte_entry)))
    (define post (inv))
    ; no UB triggered
    (for-each (lambda (x) (check-unsat? (verify (assert x)))) (asserts))
    (for-each (lambda (x) (check-unsat? (verify (assert x)))) asserted)
    ; check if the invariant holds
    (check-unsat? (verify (assert (implies pre post))))))

(define (check-paging-spec-map-l3)
  (parameterize ([current-machine (make-machine symbols globals)])
    ; Verifying that the free_frame function does not violate the invariant
    (define-symbolic l4e (bitvector PAGE_NUMBER_TYPE_SIZE))
    (define-symbolic offset (bitvector 16))
    (define-symbolic l3e (bitvector PAGE_NUMBER_TYPE_SIZE))
    (define pre (inv))
    (define asserted
      (with-asserts-only (@map_page_table_l3_entry l4e offset l3e)))
    (define post (inv))
    ; no UB triggered
    (for-each (lambda (x) (check-unsat? (verify (assert x)))) (asserts))
    (for-each (lambda (x) (check-unsat? (verify (assert x)))) asserted)
    ; check if the invariant holds
    (check-unsat? (verify (assert (implies pre post))))))

(define (check-paging-spec-map-l2)
  (parameterize ([current-machine (make-machine symbols globals)])
    ; Verifying that the free_frame function does not violate the invariant
    (define-symbolic l3e (bitvector PAGE_NUMBER_TYPE_SIZE))
    (define-symbolic offset (bitvector 16))
    (define-symbolic l2e (bitvector PAGE_NUMBER_TYPE_SIZE))
    (define pre (inv))
    (define asserted
      (with-asserts-only (@map_page_table_l2_entry l3e offset l2e)))
    (define post (inv))
    ; no UB triggered
    (for-each (lambda (x) (check-unsat? (verify (assert x)))) (asserts))
    (for-each (lambda (x) (check-unsat? (verify (assert x)))) asserted)
    ; check if the invariant holds
    (check-unsat? (verify (assert (implies pre post))))))

(define (check-paging-spec-map-l1)
  (parameterize ([current-machine (make-machine symbols globals)])
    ; Verifying that the free_frame function does not violate the invariant
    (define-symbolic l2e (bitvector PAGE_NUMBER_TYPE_SIZE))
    (define-symbolic offset (bitvector 16))
    (define-symbolic l1e (bitvector PAGE_NUMBER_TYPE_SIZE))
    (define pre (inv))
    (define asserted
      (with-asserts-only (@map_page_table_l1_entry l2e offset l1e)))
    (define post (inv))
    ; no UB triggered
    (for-each (lambda (x) (check-unsat? (verify (assert x)))) (asserts))
    (for-each (lambda (x) (check-unsat? (verify (assert x)))) asserted)
    ; check if the invariant holds
    (check-unsat? (verify (assert (implies pre post))))))

(define (check-paging-spec-map-frame)
  (parameterize ([current-machine (make-machine symbols globals)])
    ; Verifying that the free_frame function does not violate the invariant
    (define-symbolic l1e (bitvector PAGE_NUMBER_TYPE_SIZE))
    (define-symbolic offset (bitvector 16))
    (define-symbolic frame (bitvector PAGE_NUMBER_TYPE_SIZE))
    (define pre (inv))
    (define asserted
      (with-asserts-only (@map_page_table_frame l1e offset frame)))
    (define post (inv))
    ; no UB triggered
    (for-each (lambda (x) (check-unsat? (verify (assert x)))) (asserts))
    (for-each (lambda (x) (check-unsat? (verify (assert x)))) asserted)
    ; check if the invariant holds
    (check-unsat? (verify (assert (implies pre post))))))

(define allocation-tests
  (test-suite+
   "Tests for all.c"
   (test-case+ "check-paging-spec-map-l3" (check-paging-spec-map-l3))
   (test-case+ "check-paging-spec-map-l2" (check-paging-spec-map-l2))
   (test-case+ "check-paging-spec-map-l1" (check-paging-spec-map-l1))
   (test-case+ "check-paging-spec-map-frame" (check-paging-spec-map-frame))
   (test-case+ "check-paging-spec-unmap" (check-paging-spec-unmap))
   (test-case+ "check-allocation-spec-allocate" (check-allocation-spec-allocate))
   (test-case+ "check-allocation-spec-free" (check-allocation-spec-free))
   ; (test-case+ "check-allocation-spec-init" (check-allocation-spec-init)) ; commented out because it's too long
   )

  )

(module+ test
  (time (run-tests allocation-tests)))
