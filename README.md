# Crypt-Decrypt

This is a simple program to examplify how to encrypt and decrypt files using the `Sodium` library.

### How to build:
```
docker build -t file_enc_dec .
```

### How to use:
```
docker run -v "${PWD}:/opt/src" -it file_enc_dec -i /opt/src/fileToEncrypt
```

### Examples:
```
example.sh
```
