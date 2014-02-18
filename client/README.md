Hatohol Client
==============

Hatohol Client is a frontend part of Hatohol and provides a web UI based
on Django.

Required software and settings
----------------------------------------
- Python 2.6
- Django 1.5
- JavaScript enabled (web browser side)

Containing modules
----------------------------------------
This application contains following foreign modules. Thanks a lot!
- jQuery 1.9.1 (http://jquery.com/)
- Bootstrap 3.1.0 (http://getbootstrap.com/)
- Stupid-table-plugin
maintained by [joequery]
(http://joequery.github.io/Stupid-Table-Plugin/)

How to setup
----------------------------------------
> See also [this page](../doc/install/hatohol0.1-centos6.4.md)
> to setup Hatohol for CentOS with the binary packages.

### Install Hatohol Client
Hatohol Client is written as PATH FREE. So you can place it anywhere.

### Install Required Libraries
The following shows examples to install Required Packages.

#### On CentOS 6.4

    # yum install python-setuptools python-devel
    # easy_install pip
    # pip install django==1.5.4 mysql-python

#### On Ubuntu 12.04 and 13.10

    # apt-get install python-pip python-dev gettext
    # pip install django==1.5.4 mysql-python


### Create the database

    > CREATE DATABASE hatohol_client;
    > GRANT ALL PRIVILEGES ON hatohol_client.* TO hatohol@localhost IDENTIFIED BY 'hatohol';

### Create the database and the tables

    $ ./manage.py syncdb

How to run
----------------------------------------
Hatohol Client is a standard Django project. So you can run it on any WSGI
compliant application server.

Alternatively you can run with a runserver sub-command of Django's manage.py.

	$ ./manage.py runserver

If you allow to access from the outside, you need specifying the address like

	$ ./manage.py runserver 0.0.0.0:8000

Hints
-----
### How to set a Hatohol server address and the port
Edit hatohol/hatoholserver.py and update the following lines.

    DEFAULT_SERVER_ADDR = 'localhost'
    DEFAULT_SERVER_PORT = 33194
