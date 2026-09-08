// Read-only native asset checks. GPU upload uses block-compressed mip chains;
// checking the base dimension alone missed the 1256 -> 628 -> 314 failure.
const fs=require('fs'),path=require('path'),assert=require('assert');
const root=path.resolve(__dirname,'../..'),repo=path.resolve(root,'../../..');
const folder=path.join(repo,'Data/Provenance/TreeTrial/v002');
const manifest=JSON.parse(fs.readFileSync(path.join(root,'Verification/Fixtures/TreeTrialResources.json'),'utf8'));
let textures=0,masked=0;
for(const name of fs.readdirSync(folder)){
 if(name.endsWith('.png')){
  const b=fs.readFileSync(path.join(folder,name));
  assert.equal(b.subarray(1,4).toString(),'PNG');
  const w=b.readUInt32BE(16),h=b.readUInt32BE(20);
  assert(w>0&&h>0&&!(w&(w-1))&&!(h&(h-1)),`${name}: require power-of-two upload-safe mip chain`);
  ++textures;
 }
 if(name.endsWith('.material')){
  const s=fs.readFileSync(path.join(folder,name),'utf8');
  if(s.includes('AlphaTest')){assert(s.includes('TwoSided'));assert(s.includes('m_alphaCutoff'));++masked;}
 }
}
const map=fs.readFileSync(path.join(repo,'Data/Provenance/ProvenanceSandbox.map'),'utf8');
assert.equal(manifest.summary.length,14);
for(const asset of manifest.summary){
 assert(map.includes('TreeTrial_'+asset.id),asset.id+' missing map entity');
 assert(map.includes(asset.id+'.mesh'),asset.id+' missing native mesh');
}
assert.equal(masked,14);assert.equal(textures,15);
console.log(`PASS ${textures} power-of-two runtime textures; ${masked} masked foliage materials; ${manifest.summary.length} mapped tree instances; ${manifest.summary.reduce((s,a)=>s+a.triangles,0)} triangles`);
