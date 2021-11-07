FROM debian:11

# Source
RUN apt update && \
    apt install -y software-properties-common && \
    apt-add-repository 'deb https://mirrors.tuna.tsinghua.edu.cn/debian/ testing main contrib non-free' && \
    apt-add-repository 'deb https://mirrors.tuna.tsinghua.edu.cn/debian/ testing-updates main contrib non-free' && \
    apt-add-repository 'deb https://mirrors.tuna.tsinghua.edu.cn/debian/ testing-backports main contrib non-free' && \
    apt-add-repository 'deb https://mirrors.tuna.tsinghua.edu.cn/debian-security testing-security main contrib non-free'

# Updates
RUN apt update && \
    apt upgrade -y

# SSH Initial
RUN apt install -y openssh-server && \
    mkdir /run/sshd

# Dev Dependencies
RUN apt install -y gcc-11 automake autoconf libtool make gdb valgrind man-db python3 python gcc-11-multilib

# Files
COPY authorized_keys /root/.ssh/authorized_keys
COPY sshd_config /etc/ssh/sshd_config

WORKDIR /workspace

VOLUME /workspace
VOLUME /root/.vscode-server

EXPOSE 22

CMD ["/usr/sbin/sshd", "-D"]
