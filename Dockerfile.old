FROM gcc:11.2

# Updates
RUN apt update && \
    apt upgrade -y

# SSH Initial
RUN apt install -y openssh-server && \
    mkdir /run/sshd

# Dev Dependencies
RUN apt install -y gdb valgrind man-db python3 python

# Files
COPY authorized_keys /root/.ssh/authorized_keys
COPY sshd_config /etc/ssh/sshd_config

WORKDIR /workspace

VOLUME /workspace
VOLUME /root/.vscode-server

EXPOSE 22

CMD ["/usr/sbin/sshd", "-D"]
