// Source-contract checks, not a GPU image or performance certificate.
const fs = require('node:fs');
const path = require('node:path');
const root = path.resolve(__dirname, '../../../../..');
const read = p => fs.readFileSync(path.join(root, p), 'utf8')
    .replace(/\/\*[\s\S]*?\*\//g, '').replace(/\/\/[^\n]*/g, '');
const grass = read('Code/Engine/Render/Shaders/Materials/ProvenanceGrassSoilPBR.esf');
const pbr = read('Code/Engine/Render/Shaders/Renderer/MaterialShaderPBR.esh');
const start = grass.indexOf('[branch] if(isGrassBlade)');
const end = grass.indexOf('return materialOutput;', start);
if (start < 0 || end < 0) throw new Error('Grass branch not found');
const blades = grass.slice(start, end);
let checks = 0;
function check(ok, message) { ++checks; if (!ok) throw new Error(message); }
check(!blades.includes('RENDERER_GLOBAL_FLAG_DISABLE_CAST_SHADOWS'), 'Grass must receive directional shadows');
check(blades.includes('RENDERER_GLOBAL_FLAG_DISABLE_SSAO'), 'Thin-card SSAO protection retained');
check(/m_shadowNormal\s*=\s*float3\(0\.0F,0\.0F,1\.0F\)/.test(blades), 'Grass shadow bias uses world up, not encoded downward identity');
check(/m_shadowNormal\s*=\s*materialInput\.m_vertex\.m_worldNormal/.test(pbr), 'Other materials retain geometric shadow bias');
check(/SampleShadow\([^;]*vertex\.m_worldPosition[^;]*materialOutput\.m_shadowNormal/.test(pbr), 'Shadow sampling uses displaced world position and explicit bias normal');
check(!/m_shadowNormal\s*=\s*-/.test(pbr), 'Two-sided lighting must not flip receiver bias into ground');
check(/ComputeDirectLight\([^;]*\)\s*\*\s*shadowDir/.test(pbr), 'Shadow attenuation applies to direct sun');
check(/result\.rgb\s*=\s*IBL\s*;/.test(pbr), 'Environment lighting preserved');
check(/m_opacity\s*=\s*blade\.a\s*\*\s*visible/.test(blades), 'Atlas cutout and cover visibility preserved');
check(grass.includes('vertex.m_worldPosition.xy+=amount*'), 'Wind deformation preserved');
const materialsDir = path.join(root, 'Code/Engine/Render/Shaders/Materials');
for (const name of fs.readdirSync(materialsDir).filter(n => n.endsWith('.esf'))) {
    const source = read('Code/Engine/Render/Shaders/Materials/' + name);
    if (source.includes('MaterialShaderMain'))
        check(source.includes('MaterialShaderOutput::New( materialInput )'), name + ' initializes receiver bias');
}
console.log(`Grass shadow receiving: ${checks} source-contract checks passed. GPU appearance/FPS require live validation.`);
