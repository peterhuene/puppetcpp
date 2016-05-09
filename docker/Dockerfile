FROM puppetcpp-node
MAINTAINER Peter Huene <peterhuene@gmail.com>
RUN apk add --no-cache git python make gcc g++ && \
    git clone https://github.com/peterhuene/puppet-repl-server.git /tmp/puppet-repl-server && \
    cd /tmp/puppet-repl-server && \
    npm install && \
    apk --no-cache del git python make gcc g++
ENTRYPOINT []
CMD node /tmp/puppet-repl-server/server.js
