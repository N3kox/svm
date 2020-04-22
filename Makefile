avsvm: env/avsvm_slp.c
	gcc env/avsvm_slp.c -o avsvm

.PHONY:clean
clean:
	rm -rf *.o avsvm