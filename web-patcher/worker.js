importScripts('./vendor/HashCalculator.js', './vendor/BinFile.js', './vendor/RomPatcher.format.bps.js');
const builder = import('./builder.mjs');

self.onmessage = async ({data}) => {
  try {
    const {build3dsx} = await builder;
    const output = await build3dsx(data.rom, data.manifest, async name => {
      const response = await fetch(new URL('assets/' + name, self.location.href));
      if (!response.ok) throw new Error('A local build file is unavailable.');
      return response.arrayBuffer();
    }, progress => self.postMessage({progress}));
    self.postMessage({output: output.buffer}, [output.buffer]);
  } catch (error) {
    self.postMessage({error: error.message});
  }
};
