# web-ui

Install node.js:

```bash
curl -sL https://deb.nodesource.com/setup_12.x | sudo bash
sudo apt-get install -y nodejs
node --version
npm --version
```

Build the web ui:

```bash
# install dependencies.
npm install

# build in production mode.
npm run build
```

If you want to test the development build, start the web server
at http://localhost:8080 with:

```bash
npm run dev
```

If you want to test the production build, start the web server
at http://localhost:8080 with:

```bash
npm run serve
```

For detailed explanation on how things work, checkout the [preact cli readme](https://github.com/developit/preact-cli/blob/master/README.md).


Build the firmware filesystem image and upload it to the device with:

```bash
source ../venv/bin/activate
./build.sh
platformio run --target uploadfs
```
