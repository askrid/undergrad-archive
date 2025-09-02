# Grading

1. Run docker container

```bash
sudo docker run -it jihoonjang2/automata:hw2 bin/bash
```

2. Access to the container

```bash
sudo docker ps -a
sudo docker start CONTAINER_ID
sudo docker attach CONTAINER_ID
```

3. Move local files to the container

```bash
sudo docker cp "source.zip" CONTAINER_ID:/home/hw3/submissions/
```

4. Run test script

```bash
cd /home/hw3
python3 grader.py
```
