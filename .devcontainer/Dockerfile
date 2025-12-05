FROM gcc:14

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    cmake git build-essential gdb libgtest-dev tree\
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

CMD ["bash"]
