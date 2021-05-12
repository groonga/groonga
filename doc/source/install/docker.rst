.. -*- rst -*-
.. Groonga Project

Docker
=======

This section describes how to install Groonga on Docker. You can
install Groonga image via DockerHub.

We distribute Alpine Linux Groonga docker image on DockerHub.

Pulling image
-------------

Install::

  % docker pull groonga/groonga:latest

Then run it with the following command::

  % docker run -v /mnt/db:/path/to/db groonga/groonga /mnt/db


With docker-compose
-------------------

Create docker-compose.yml as follows::

  version: '3'
  services:
    groonga:
      image: groonga/groonga
      volumes:
        - ./groonga:/mnt/db
      ports:
        - "10041:10041"
      command: ["-n", "/mnt/db/data.db"]

Then run it with the following command::

  % docker-compose run groonga
