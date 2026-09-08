FROM ghcr.io/dragonminded/libdragon:latest-arm64
ARG LIBDRAGON_COMMIT=c4a7e119eff1cfad07adcfa892a2910c40d8bdb8
RUN git init /tmp/libdragon && cd /tmp/libdragon \
 && git remote add origin https://github.com/DragonMinded/libdragon.git \
 && git fetch --depth 1 origin ${LIBDRAGON_COMMIT} && git checkout FETCH_HEAD \
 && make -j$(nproc) install-mk \
 && make -j$(nproc) libdragon tools \
 && make -j$(nproc) install tools-install \
 && rm -rf /tmp/libdragon
