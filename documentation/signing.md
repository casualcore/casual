# Signing
## Overview
In order to keep casual rpms as secure as possible we are using **pgp** to sign the packages.

## Prerequisite
The casual official private signing key is installed in your keychain. The key and the passphrase can be received from responsible party in the community.
Use gpg to import/install private key.

### Public key-id
330916DA5D304C2B84DD94CE15362C063E05B561

which is published to central keyservers, e.g. https://keyserver.ubuntu.com/

## Signing
```
$ rpmsign --key-id 330916DA5D304C2B84DD94CE15362C063E05B561 --addsign <rpmfile-to-sign>
```
Enter *secret* passphrase when asked.

Note! Both file and catalog needs to be writable to be able to sign. It may be a good idea to copy the files elsewhere and then after signing copying them back.

## Checking package
```
$ rpm -qip <signed-rpmfile> | grep Signature
```