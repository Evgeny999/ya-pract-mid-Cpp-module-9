FROM ubuntu@sha256:79efa276fdefa2ee3911db29b0608f8c0561c347ec3f4d4139980d43b168d991

# Update the package database
RUN apt clean
RUN apt update && apt upgrade -y

# Install common tools
RUN apt install -y python3-minimal python3-pip pipx git less vim sudo cmake make g++-15 clang-format clangd-20

# Install project required tools
RUN apt install -y libsfml-dev

# Cleanup
RUN apt autoremove -y \
    && apt autoclean \
    && rm -rf /var/lib/apt/lists/* \
    && rm -rf /var/cache/apt/* \
    && rm -rf /tmp/*

# Update  gcc/g++ version to the latest ones
RUN ln -fs /usr/bin/g++-15 /usr/bin/g++
RUN ln -fs /usr/bin/gcc-15 /usr/bin/gcc

# Delete Ubuntu user to avoid "UID/GID is already exists" problems
# Create a non-root user 'dev'
RUN useradd -ms /bin/bash dev \
    && echo "dev ALL=(ALL) NOPASSWD: ALL" >> /etc/sudoers.d/dev \
    && chmod 0440 /etc/sudoers.d/dev

# Switch to non-root user
USER $USERNAME

RUN git config --global core.editor code

# Install conan through pipx
RUN pipx install conan
RUN pipx ensurepath

# Set the default command
CMD ["/bin/bash"]
