# Use Alpine Linux 3 as the base image
FROM alpine:3

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

# Final stage
FROM busybox:stable-musl

COPY --from=0 /app/UsagiInit /UsagiInit

# Set default command
ENTRYPOINT ["/UsagiInit"]
