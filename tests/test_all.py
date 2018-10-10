#!/usr/bin/python3
import os
import time

import requests


HOST = 'http://localhost:18080'


def test_creation_and_extension():
    print(' === Testing creation of the link, and the following prolongation ===')
    r = requests.post(
        HOST + '/link',
        json={
            'original_url': 'https://www.yandex.ru',
            'ttl': 10,
        },
        timeout=1,
    )
    print('Got response with the status code:', r.status_code)
    assert r.status_code == 201, 'the status code is not 201'

    location = r.headers.get('Location')
    print('The LOCATION header is:', location)
    assert isinstance(location, str) and location.startswith(HOST)

    """
    The full cycle of the cleanup is set to 120 sec.
    So if we do 30 requests with the delay of 5s., we'll definitely catch it.
    """
    print(' = Querying the link by the acquired location =')
    for _ in range(30):
        r = requests.get(location, timeout=1)
        print('Iteration {}. Got response with the status code:'.format(_), r.status_code)
        assert r.status_code == 200, 'the status code is not 200'
        time.sleep(5)


def test_expiration():
    print(' === Testing expiration of the link ===')
    r = requests.post(
        HOST + '/link',
        json={
            'original_url': 'https://www.yandex.ru',
            'ttl': 10,
        },
        timeout=1,
    )
    print('Got response with the status code:', r.status_code)
    assert r.status_code == 201, 'the status code is not 201'

    location = r.headers.get('Location')
    print('The LOCATION header is:', location)
    assert isinstance(location, str) and location.startswith(HOST)

    print(' = Querying the link by the acquired location right after posting =')
    r = requests.get(location, timeout=1)
    print('Got response with the status code:', r.status_code)
    assert r.status_code == 200, 'the status code is not 200'

    print(' = Sleeping 2x TTL =')
    time.sleep(20)

    print(' = Querying again, expecting 404 =')
    r = requests.get(location, timeout=1)    
    print('Got response with the status code:', r.status_code)
    assert r.status_code == 404, 'the status code is not 404'


if __name__ == '__main__':
    test_creation_and_extension()
    test_expiration()

