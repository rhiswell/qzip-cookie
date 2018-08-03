#!/bin/bash

set -ueo pipefail

CALGARY_URL=http://www.data-compression.info/files/corpora/largecalgarycorpus.zip

function prepare_calgary
{
    wget -O /tmp/largecalgary.zip $CALGARY_URL && \
        unzip -c /tmp/largecalgary.zip "*" > calgary && \
        rm /tmp/largecalgary.zip
}

test -e calgary || prepare_calgary
