// `npm run build` uses this code path:
//
//  https://github.com/preactjs/preact-cli/blob/v3.0.0/packages/cli/lib/index.js#L33-L63
//  https://github.com/preactjs/preact-cli/blob/v3.0.0/packages/cli/lib/commands/build.js#L30
//  https://github.com/preactjs/preact-cli/blob/v3.0.0/packages/cli/lib/lib/webpack/run-webpack.js#L265
//  https://github.com/preactjs/preact-cli/blob/v3.0.0/packages/cli/lib/lib/webpack/run-webpack.js#L85
//  https://github.com/preactjs/preact-cli/blob/v3.0.0/packages/cli/lib/lib/webpack/webpack-client-config.js#L27
//  https://github.com/preactjs/preact-cli/blob/v3.0.0/packages/cli/lib/lib/webpack/transform-config.js#L93

const ZopfliPlugin = require("zopfli-webpack-plugin");

// NB config is-a https://webpack.js.org/configuration/#options
export default function (config, env, helpers) {
  if (env.isProd) {
    // disable sourcemaps.
    config.devtool = false;

    // // dump plugin names.
    // for (const plugin of config.plugins) {
    //   const name = plugin.constructor && plugin.constructor.name;
    //   console.log("webpack plugin:", name);
    // }

    // compress the assets.
    config.plugins.push(
      new ZopfliPlugin({
        deleteOriginalAssets: true,
        minRatio: 0,
        // NB .html files are not really included in the processing pipeline,
        //    so we process index.html from build.sh.
        test: /\.(js|css|html)$/i,
      })
    );
  }
}