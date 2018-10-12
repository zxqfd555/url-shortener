#!/usr/bin/python3
# -*- coding: utf-8 -*-

# ~ Courtesy of https://yandextank.readthedocs.io/en/latest/ammo_generators.html ~

import json
import random
import sys

import numpy


def make_ammo(method, url, headers, case, body):
    """ makes phantom ammo """
    #http request w/o entity body template
    req_template = (
          "%s %s HTTP/1.1\n"
          "%s\n"
          "\n"
    )

    #http request with entity body template
    req_template_w_entity_body = (
          "%s %s HTTP/1.1\n"
          "%s\n"
          "Content-Length: %d\n"
          "\n"
          "%s\n"
    )

    if not body:
        req = req_template % (method, url, headers)
    else:
        req = req_template_w_entity_body % (method, url, headers, len(body), body)

    #phantom ammo template
    ammo_template = (
        "%d %s\n"
        "%s"
    )

    return ammo_template % (len(req), case, req)


def generate(lines_count):
    total_shortened_links = 0
    always_prolongation = 0
    small_ttl = 0

    for _ in range(lines_count):
        if random.randint(0, 99) > 98 or total_shortened_links == 0:
            headers = """Content-Type: application/json
Host: localhost:18080"""

            ttl = random.randint(10, 3600 * 3)
            if ttl < 90:
                always_prolongation += 1
            elif ttl <= 300:
                small_ttl += 1
            payload = {
                'original_url': 'http://localhost',
                'ttl': ttl,
            }
            body = json.dumps(payload)
            sys.stdout.write(make_ammo('POST', '/link', headers, 'the_only_tag', body))
            total_shortened_links += 1
        else:
            headers = 'Host: localhost:18080'
            link_number = numpy.random.poisson(1) % total_shortened_links + 1
            if link_number == 0:
                link_number += 1
            sys.stdout.write(make_ammo('GET', '/{}'.format(link_number), headers, 'the_only_tag', ''))

    sys.stderr.write('always prolongation: {}'.format(always_prolongation))
    sys.stderr.write('small ttl: {}'.format(small_ttl))
    sys.stderr.write('total created: {}'.format(total_shortened_links))

if __name__ == "__main__":
    generate(2 * 10**6)

