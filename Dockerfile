FROM ubuntu:22.04

RUN apt-get update -qq && \
    apt-get install -y --no-install-recommends \
        build-essential make python3 curl siege netcat-openbsd nginx && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /webserv
COPY . .

RUN make fclean && make -j$(nproc)

EXPOSE 1818

CMD ["./webserv", "config/default.conf"]
