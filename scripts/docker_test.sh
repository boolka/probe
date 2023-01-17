docker build -t localhost/probe_test -f docker/Dockerfile.test .
docker run -it --rm -p 80:80 localhost/probe_test
