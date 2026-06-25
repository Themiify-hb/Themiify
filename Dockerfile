FROM dkosmari/devkitppc-wiiu-debian
# FROM devkitpro/devkitppc

# NOTE: If you use upstream devkitppc, you may need additional packages.
# RUN apt install -y automake libtool

# NOTE: If you use upstream devkitppc, you may need to manually update the devkitPro packages.
# RUN dkp-pacman -Syu --noconfirm

# NOTE: If you use upstream devkitppc, everything runs as root, so comment out this line.
USER user

WORKDIR /project

# NOTE: If you use upstream devkitppc, remove the --chown option.
COPY --chown=user:user . /project
