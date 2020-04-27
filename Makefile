all: slp le
slp: env/avsvm_slp.c
	gcc env/avsvm_slp.c -o slp
le: env/avsvm_le.c
	gcc env/avsvm_le.c -o le

.PHONY:clean
clean:
	rm slp le
