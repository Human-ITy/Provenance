// Production texture assembly: periodic overlap quilting, no rescaling of source.
const fs=require('fs'),path=require('path');
const sharp=require('C:/Users/D-Day/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/node_modules/sharp');
const dir=__dirname, N=1024, feather=3;
const source='C:/Users/D-Day/.codex/generated_images/01a07c7d-50dd-7712-9ff7-9bdb2ceb9134/exec-9df43890-4bd9-4c6f-a1d3-a8e4c435c91e.png';
function cut(cost,rows,cols,closed=false){
  const parent=new Int16Array(rows*cols);
  let prev=new Float64Array(cols),next=new Float64Array(cols);
  const start=Math.floor(cols/2);
  for(let c=0;c<cols;c++)prev[c]=closed&&c!==start?1e20:cost[c];
  for(let r=1;r<rows;r++){
    for(let c=0;c<cols;c++){
      let best=c;
      if(c>0&&prev[c-1]<prev[best])best=c-1;
      if(c+1<cols&&prev[c+1]<prev[best])best=c+1;
      next[c]=prev[best]+cost[r*cols+c];parent[r*cols+c]=best;
    }
    [prev,next]=[next,prev];
  }
  let last=start;
  if(!closed)for(let c=0;c<cols;c++)if(prev[c]<prev[last])last=c;
  const seam=new Int16Array(rows);seam[rows-1]=last;
  for(let r=rows-1;r>0;r--)seam[r-1]=parent[r*cols+seam[r]];
  return seam;
}
function stats(a,w,h){
  const vertical=[],horizontal=[],allx=[],ally=[];
  const d=(i,j)=>(Math.abs(a[i]-a[j])+Math.abs(a[i+1]-a[j+1])+Math.abs(a[i+2]-a[j+2]))/3;
  for(let y=0;y<h;y++){
    vertical.push(d(y*w*3,(y*w+w-1)*3));
    for(let x=1;x<w;x++)allx.push(d((y*w+x)*3,(y*w+x-1)*3));
  }
  for(let x=0;x<w;x++){
    horizontal.push(d(x*3,((h-1)*w+x)*3));
    for(let y=1;y<h;y++)ally.push(d((y*w+x)*3,((y-1)*w+x)*3));
  }
  const mean=a=>a.reduce((s,v)=>s+v,0)/a.length;
  return {wrap_x_mean:mean(vertical),wrap_y_mean:mean(horizontal),internal_x_mean:mean(allx),internal_y_mean:mean(ally)};
}
(async()=>{
  fs.copyFileSync(source,path.join(dir,'creek_bank_mud_v001_refined_source.png'));
  const {data:a,info}=await sharp(source).removeAlpha().raw().toBuffer({resolveWithObject:true});
  const W=info.width,H=info.height,ox=W-N,oy=H-N;
  if(ox<64||oy<64)throw Error('Insufficient source overlap');
  // Restrict the cut to the middle of the overlap, guaranteeing wrap continuity.
  const low=32,cw=ox-64,cost=new Float32Array(H*cw);
  for(let y=0;y<H;y++)for(let c=0;c<cw;c++){
    const x=c+low,i=(y*W+x)*3,j=(y*W+x+N)*3;
    let e=0;for(let k=0;k<3;k++)e+=(a[i+k]-a[j+k])**2;
    cost[y*cw+c]=e;
  }
  const sx=cut(cost,H,cw),b=Buffer.alloc(N*H*3);
  for(let y=0;y<H;y++)for(let x=0;x<N;x++){
    const t=x<ox?Math.max(0,Math.min(1,(x-(sx[y]+low)+feather)/(2*feather))):1;
    for(let k=0;k<3;k++)b[(y*N+x)*3+k]=Math.round(x<ox?a[(y*W+x+N)*3+k]*(1-t)+a[(y*W+x)*3+k]*t:a[(y*W+x)*3+k]);
  }
  const ch=oy-64,cy=new Float32Array(N*ch);
  for(let x=0;x<N;x++)for(let c=0;c<ch;c++){
    const y=c+low,i=(y*N+x)*3,j=((y+N)*N+x)*3;
    let e=0;for(let k=0;k<3;k++)e+=(b[i+k]-b[j+k])**2;
    cy[x*ch+c]=e;
  }
  // Closed seam: same overlap selection at left and right tile edges.
  const sy=cut(cy,N,ch,true),out=Buffer.alloc(N*N*3);
  for(let y=0;y<N;y++)for(let x=0;x<N;x++){
    const t=y<oy?Math.max(0,Math.min(1,(y-(sy[x]+low)+feather)/(2*feather))):1;
    for(let k=0;k<3;k++)out[(y*N+x)*3+k]=Math.round(y<oy?b[((y+N)*N+x)*3+k]*(1-t)+b[(y*N+x)*3+k]*t:b[(y*N+x)*3+k]);
  }
  const final=path.join(dir,'creek_bank_mud_v001_seamless_color.png');
  await sharp(out,{raw:{width:N,height:N,channels:3}}).png().toFile(final);
  const thumbnail=await sharp(final).resize(512,512).toBuffer(),patches=[];
  for(let y=0;y<3;y++)for(let x=0;x<3;x++)patches.push({input:thumbnail,left:x*512,top:y*512});
  await sharp({create:{width:1536,height:1536,channels:3,background:'#000'}}).composite(patches).jpeg({quality:96}).toFile(path.join(dir,'creek_bank_mud_v001_repeat_review.jpg'));
  // Native-resolution wrap junction crops: each panel straddles an actual tile boundary.
  const size=384,join=Buffer.alloc(size*size*3);
  for(let y=0;y<size;y++)for(let x=0;x<size;x++){
    const yy=(y-size/2+N)%N,xx=(x-size/2+N)%N;
    out.copy(join,(y*size+x)*3,(yy*N+xx)*3,(yy*N+xx)*3+3);
  }
  await sharp(join,{raw:{width:size,height:size,channels:3}}).png().toFile(path.join(dir,'creek_bank_mud_v001_corner_join.png'));
  const report={source_size:[W,H],final_size:[N,N],opaque:true,upscaled:false,method:'Image-gen source followed by periodic minimum-error overlap quilting; 3px seam feather; closed second-axis cut.',source:stats(a,W,H),final:stats(out,N,N),seam_x_range:[Math.min(...sx)+low,Math.max(...sx)+low],seam_y_range:[Math.min(...sy)+low,Math.max(...sy)+low],closed_second_axis_seam:sy[0]===sy[N-1]};
  fs.writeFileSync(path.join(dir,'creek_bank_mud_v001_inspection.json'),JSON.stringify(report,null,2));
  console.log(JSON.stringify(report,null,2));
})().catch(e=>{console.error(e);process.exit(1)});

