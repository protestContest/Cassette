FROM debian

RUN apt update && apt install -y build-essential clang git libx11-dev
RUN rm -rf Cassette && git clone https://github.com/protestContest/Cassette
WORKDIR Cassette
RUN make
CMD bin/cassette -L share test/net.ct
