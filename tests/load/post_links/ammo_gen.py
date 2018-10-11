#!/usr/bin/python3
# -*- coding: utf-8 -*-

# ~ Courtesy of https://yandextank.readthedocs.io/en/latest/ammo_generators.html ~

import json
import random
import sys


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
    for _ in range(lines_count):
        headers = """Content-Type: application/json
Host: localhost:18080"""

        payload = {
            'original_url': 'http://localhost',
            'ttl': random.randint(10, 30),
        }
        body = json.dumps(payload)

        sys.stdout.write(make_ammo('POST', '/link', headers, 'the_only_tag', body))


if __name__ == "__main__":
    generate(10)

