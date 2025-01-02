============
Installation
============

-------------
Prerequisites
-------------

You'll need a Linux. OSX, BSD, or other flavour of Unix, server with a public IP address and
a shell account able to setup a daemon listening on a port. You'll need a modern version of
perl running on that server, and a bunch of perl modules (from cpan).

You'll need a MYSQL server running on the same machine, or another machine, and credentials
to be able to create a database and accounts.

This could possibly run on a windows server, but you are on your own ;-)
[ let us know instructions if you manage to get it running ]

-----------------
Clone from GitHub
-----------------

Start the installation with a clone from GitHub:

    git clone https://github.com/openvehicles/Open-Vehicle-Server.git

You can then change into the v3/server directory, and start the installation process.


------------------------
Perl Module Installation
------------------------

You will need, at a minimum, the following modules from CPAN (or from your distribution's package manager):

* Config::IniFiles
* Digest::MD5
* Digest::HMAC
* Crypt::RC4::XS
* MIME::Base64
* DBI
* DBD::mysql
* EV
* AnyEvent
* AnyEvent::HTTP
* AnyEvent::HTTPD
* HTTP::Parser::XS
* Data::UUID
* Email::MIME
* Email::Sender::Simple
* Net::SSLeay
* JSON::XS
* Protocol::WebSocket
* Time::Piece

-----------
MySQL Setup
-----------

Create a mysql database "openvehicles" and use the RetroVMS_server.sql script to create
the necessary RetroVMS_* tables. Create a mysql username and password with
access to the database.

Insert a record into the RetroVMS_cars table, to give you access to your own car:

.. code-block:: text

  vehicleid: DEMO
  owner: 1
  carpass: DEMO
  userpass: DEMO
  cryptscheme: 0
  v_ptoken:
  v_lastupdate: 0000-00-00 00:00:00

Obviously, change the vehicleid, carpass and userpass as necessary. The only required fields are
vehicleid and carpass.

If you are upgrading from server v2 to v3, you can instead source the RetroVMS_server_v2_to_v3.sql
code to migrate your database schema.

-------------------------
RetroVMS Server Configuration
-------------------------

Copy conf/RetroVMS_server.conf.default to conf/RetroVMS_server.conf to provide a template for your
configuration. Then modify the configuration as appropriate.

---------------------------
RetroVMS Server Access to MySQL
---------------------------

In the config/RetroVMS_server.conf file, define the access to the mysql database:

.. code-block:: text

  [db]
  path=DBI:mysql:database=openvehicles;host=127.0.0.1
  user=<mysqlusername>
  pass=<mysqlpassword>

If the host is remote, change the host= parameter to be the remote IP address. Use 127.0.0.1 for local.

Test the connection with:

.. code-block:: text

  $ mysql -h 127.0.0.1 -u <mysqlusername> -p
  Enter password: <mysqlpassword>
  Welcome to the MySQL monitor.  Commands end with ; or \g.

If you get "ERROR 1044 (42000): Access denied", fix it before proceeding. The most likely cause
is a mistake in your mysql user grant rights.

---------------------
Enable SSL (optional)
---------------------

If you want to support SSL connections to your server (port 6869 for the REST API, port 6870 for
RetroVMS MP), you need to supply a certificate. You can create a self-signed certificate or get a
certificate signed by some root CA for your server.

In both cases you need to merge the private key PEM and the chain PEM into the file "RetroVMS_server.pem"
located in the same directory as the "RetroVMS_server.pl". Also take care to secure the file, as it now
contains your private key.

To create a self-signed certificate, do:

.. code-block:: text

  $ openssl req -sha256 -newkey rsa:4096 -nodes -keyout privkey.pem -x509 -days 365 -out fullchain.pem
  $ cat privkey.pem fullchain.pem >conf/RetroVMS_server.pem
  $ chmod 0600 conf/RetroVMS_server.pem

Or, if you want to reuse e.g. your Let's Encrypt server certificate, do this as root:

.. code-block:: text

  # cat /etc/letsencrypt/live/yourhost/privkey.pem /etc/letsencrypt/live/yourhost/fullchain.pem >conf/RetroVMS_server.pem
  # chmod 0600 conf/vms_server.pem
  # chown youruid. conf/RetroVMS_server.pem

… and add a cron job or certbot hook to check for renewals and redo these steps as necessary.

---------------------
Configure the Plugins
---------------------

The RetroVMS Server v3 is based on a pluggable architecture. The plugins themselves are stored
in plugins/system and plugins/local directories. You must configure (in conf/RetroVMS_server.conf)
the plugins that you require.

We recommend the following:

.. code-block:: text

  [plugins]
  load=<<EOT
  VECE
  DbDBI
  AuthDrupal
  ApiV2
  Push
  PushAPNS
  PushGCM
  PushMAIL
  ApiHttp
  ApiHttpCore
  ApiHttpMqapi
  EOT

--------------
Run The Server
--------------

You can run the server manually with:

.. code-block:: text

  $ ./RetroVMS_server.pl

If your linux host is running systemd, you can also look at support/RetroVMS_server.service
and support/RetroVMS.logrotate as examples for how you can run this as a background
daemon.

7. ENJOY

Any questions/comments, please let us know.

Mark Webb-Johnson
March 2020
