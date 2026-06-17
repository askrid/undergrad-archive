.PHONY: vm ssh up down build run submit clean

vm:
	./scripts/setup.sh

ssh:
	./scripts/ssh.sh

up:
	./scripts/sync.sh up

down:
	./scripts/sync.sh down

build: up
	./scripts/ssh.sh "cd SimpleRouter/router && make"

submit:
	./scripts/submit.sh

clean:
	rm -rf submission/*.tar.gz submission/readme.pdf
