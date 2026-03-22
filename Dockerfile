# PIMeval Docker Environment
FROM ubuntu:22.04

# Avoid interactive prompts during apt-get
ENV DEBIAN_FRONTEND=noninteractive

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    g++ \
    make \
    python3 \
    git \
    libjpeg-dev \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /pimeval

# Copy project files
COPY . .

# Run setup
RUN chmod +x setup.sh && ./setup.sh

# Build the project (perf target by default)
RUN make -j$(nproc)

# Command to run check by default
CMD ["make", "check"]
