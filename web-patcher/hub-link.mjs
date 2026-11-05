// Keep the standalone native builder as a fallback during a coordinated rollout.
try {
  const response=await fetch('/FireRed-Evolved/hub-info.json',{cache:'no-store'});
  const info=response.ok?await response.json():null;
  if(info?.schema===1&&info.dualPlatform===true) location.replace('/FireRed-Evolved/?platform=3ds');
} catch { /* The qualified native builder remains available. */ }
