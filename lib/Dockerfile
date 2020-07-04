FROM ubuntu:latest

USER root
WORKDIR /root

RUN mkdir /station
ADD ./ /station/
ADD ./libHCCore.so /usr/lib
ADD ./libhcnetsdk.so /usr/lib
ADD ./libhpr.so /usr/lib

EXPOSE 8085

RUN chmod +x /station/ac
WORKDIR /station
CMD ["./ac"]
