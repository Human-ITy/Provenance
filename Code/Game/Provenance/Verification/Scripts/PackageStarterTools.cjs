// Native resource preparation for an explicitly decorative trial. Descriptors
// are emitted for reviewed apply_patch installation; only binary derivatives
// are installed by --install-binaries. Source artist files are never modified.
const fs=require('fs'),path=require('path'),zlib=require('zlib'),crypto=require('crypto');
const root=path.resolve(__dirname,'../..'),repo=path.resolve(root,'../../..');
const derived=path.join(root,'output/integration/starter_tools_v001');
const dest=path.join(repo,'Data/Provenance/Tools/StarterStone/v001');
const prefix='data://provenance/tools/starterstone/v001/';
const report=JSON.parse(fs.readFileSync(path.join(derived,'export_report.json'),'utf8'));
const xml=[],binary=[],resources=[],summary=[];
const esc=s=>String(s).replaceAll('&','&amp;').replaceAll('"','&quot;').replaceAll('<','&lt;');
function textFile(name,text){xml.push({path:path.join(dest,name),text});}
function resource(name,text){textFile(name,text);resources.push(prefix+name);return prefix+name;}
function prop(name,value){return `    <Property ID="${name}" Value="${esc(value)}" />`;}
function crc32(b){let c=0xffffffff;for(const v of b){c^=v;for(let k=0;k<8;k++)c=(c>>>1)^((c&1)?0xedb88320:0);}return(c^0xffffffff)>>>0;}
function chunk(type,b){const t=Buffer.from(type),n=Buffer.alloc(4),crc=Buffer.alloc(4);n.writeUInt32BE(b.length);crc.writeUInt32BE(crc32(Buffer.concat([t,b])));return Buffer.concat([n,t,b,crc]);}
// Constant texels encode numeric material parameters, not painted art assets.
function solidPNG(rgba){const head=Buffer.alloc(13);head.writeUInt32BE(4);head.writeUInt32BE(4,4);head[8]=8;head[9]=6;const row=Buffer.from([0,...Array(4).fill(rgba).flat()]);return Buffer.concat([Buffer.from([137,80,78,71,13,10,26,10]),chunk('IHDR',head),chunk('IDAT',zlib.deflateSync(Buffer.concat(Array(4).fill(row)))),chunk('IEND',Buffer.alloc(0))]);}
const byte=x=>Math.max(0,Math.min(255,Math.round(x*255)));
const srgb=x=>byte(x<=.0031308?12.92*x:1.055*Math.pow(x,1/2.4)-.055);
function texture(name,bytes,group){binary.push({name:name+'.png',bytes});return resource(name+'.texture',`<Type TypeID="EE::Render::TextureResourceDescriptor" Version="0">\n  <Property ID="m_sourcePaths">\n    <Property Index="0" Value="${prefix+name}.png" />\n  </Property>\n  <Property ID="m_textureGroup" Value="data://render/texturegroups/${group}.txtg" />\n</Type>\n`);}
for(const entry of report.filter(e=>!process.env.TRIAL_ASSET||e.id===process.env.TRIAL_ASSET)){
 const id=entry.id,bytes=fs.readFileSync(entry.export);let offset=12,j,bin;
 while(offset<bytes.length){const len=bytes.readUInt32LE(offset),type=bytes.readUInt32LE(offset+4),data=bytes.subarray(offset+8,offset+8+len);if(type===0x4e4f534a)j=JSON.parse(data.toString());if(type===0x004e4942)bin=data;offset+=len+8;}
 if(!j||!bin)throw Error('Invalid GLB '+id);
 for(const n of j.nodes){if(n.translation?.some(x=>Math.abs(x)>1e-6)||n.rotation||n.scale||n.matrix)throw Error('Unbaked node '+n.name);}
 binary.push({name:id+'.glb',bytes});
 const textureCache=new Map();
 function imageTexture(index,group){const imageIndex=j.textures[index].source,key=imageIndex+'/'+group;if(textureCache.has(key))return textureCache.get(key);const image=j.images[imageIndex],view=j.bufferViews[image.bufferView];if(image.mimeType!=='image/png')throw Error('Expected embedded PNG');const result=texture(id+'_image_'+imageIndex+'_'+group,bin.subarray(view.byteOffset||0,(view.byteOffset||0)+view.byteLength),group);textureCache.set(key,result);return result;}
 const mats=j.materials.map((m,i)=>{
  const name=id+'_mat_'+i,p=m.pbrMetallicRoughness||{},factor=p.baseColorFactor||[1,1,1,1];
  if(p.baseColorTexture&&factor.some(x=>x!==1))throw Error('Textured factor needs explicit mapping '+m.name);
  const albedo=p.baseColorTexture?imageTexture(p.baseColorTexture.index,'albedotexture'):texture(name+'_color',solidPNG([...factor.slice(0,3).map(srgb),byte(factor[3])]),'albedotexture');
  if(p.metallicRoughnessTexture)throw Error('Packed texture mapping not implemented');
  const pbr=texture(name+'_pbr',solidPNG([byte(p.metallicFactor??1),255,0,byte(p.roughnessFactor??1)]),'uncompressed4channels');
  const props=[prop('m_shaderFlags',m.doubleSided?'TwoSided':''),prop('m_albedoTexture',albedo),prop('m_pbrTexture',pbr)];
  if(m.normalTexture)props.push(prop('m_normalTexture',imageTexture(m.normalTexture.index,'normaltexture')));
  const e=m.emissiveFactor||[0,0,0],strength=m.extensions?.KHR_materials_emissive_strength?.emissiveStrength??1;
  // Existing shader's exposed strength is 0..1. Preserve hue, clamp strength
  // for this trial; no engine shader changes or implicit point lights.
  if(e.some(x=>x>0)){
   if(m.emissiveTexture&&e.some(x=>Math.abs(x-e[0])>1e-7))throw Error('Colored emissive texture factor needs explicit mapping');
   const emission=m.emissiveTexture?imageTexture(m.emissiveTexture.index,'albedotexture'):texture(name+'_emission',solidPNG([...e.map(srgb),255]),'albedotexture');
   props.push(prop('m_emissiveTexture',emission),prop('m_emissiveStrength',Math.min(1,strength*(m.emissiveTexture?e[0]:1))));
  }
  return resource(name+'.material',`<Type TypeID="EE::Render::MaterialResourceDescriptor" Version="0">\n  <Property ID="m_shader" Value="ComplexSurfacePBR" />\n  <Type ID="m_shaderParameters" TypeID="EE::Render::Shaders::ComplexSurfacePBRParameters">\n${props.join('\n')}\n  </Type>\n</Type>\n`);
 });
 const mappings=[];let triangles=0;
 for(const n of j.nodes)if(n.mesh!==undefined)for(const p of j.meshes[n.mesh].primitives){if(p.mode!==undefined&&p.mode!==4)throw Error('Not triangles');const m=j.materials[p.material];mappings.push({id:n.name+'/'+m.name+'/1',material:mats[p.material]});triangles+=j.accessors[p.indices].count/3;}
 const unique=[...new Map(mappings.map(m=>[m.id,m])).values()];
 resource(id+'.mesh',`<Type TypeID="EE::Render::StaticMeshResourceDescriptor" Version="2">\n  <Property ID="m_meshPath" Value="${prefix+id}.glb" />\n  <Property ID="m_materialMappings">\n${unique.map((m,i)=>`    <Type Index="${i}" TypeID="EE::Render::MeshMaterialMapping">\n${prop('m_mappingID',m.id)}\n${prop('m_material',m.material)}\n    </Type>`).join('\n')}\n  </Property>\n  <Property ID="m_meshGroup" Value="data://render/meshgroups/editormesh.meshgrp" />\n</Type>\n`);
 summary.push({id,triangles,submeshes:mappings.length,materials:mats.length,bounds:entry.bounds_z_up,source_sha256:entry.source_sha256});
}
if(process.argv.includes('--install-binaries')){
 fs.mkdirSync(dest,{recursive:true});
 for(const b of binary){const target=path.join(dest,b.name);if(fs.existsSync(target)&&!fs.readFileSync(target).equals(b.bytes))throw Error('Refusing to overwrite different file '+target);fs.writeFileSync(target,b.bytes);}
 console.log(JSON.stringify({installedBinaryFiles:binary.length,destination:dest}));
}else if(process.argv.includes('--summary')){
 console.log(JSON.stringify({resources,summary,binaries:binary.map(b=>({name:b.name,sha256:crypto.createHash('sha256').update(b.bytes).digest('hex')}))}));
}else{
 console.log(JSON.stringify({files:xml,resources,summary,binaries:binary.map(b=>({name:b.name,sha256:crypto.createHash('sha256').update(b.bytes).digest('hex')}))}));
}
