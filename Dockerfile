## Compile executable
FROM alpine:3 AS compiler

# Install required packages
RUN apk add --no-cache cmake gcc make bash musl-dev

# Set the working directory inside the container
WORKDIR /app/UsagiInit

# Copy the entire project directory into the container
COPY . .

# Make sure the script is executable
RUN chmod +x scripts/release_static.sh

# Run the build script
RUN ./scripts/release_static.sh

## Compress executable
FROM rexezugedockerutils/upx AS upx

FROM debian:stable-slim AS compressor

COPY --from=compiler /app/UsagiInit/UsagiInit /UsagiInit

COPY --from=upx /upx /usr/local/bin/upx

RUN upx --best --lzma /UsagiInit

# Final stage
FROM busybox:stable-musl

COPY --from=compressor /UsagiInit /UsagiInit

# Set default command
ENTRYPOINT ["/UsagiInit"]
