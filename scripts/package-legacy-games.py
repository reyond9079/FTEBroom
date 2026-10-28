"""Bundle verified game tools and corresponding sources from a prepared cache."""
from pathlib import Path
import hashlib,json,shutil
def bundle(source,root,platform):
    cache=Path(source)/'deps/legacy-games'
    manifest=json.loads((cache/'manifest.json').read_text(encoding='utf-8'))
    for group in ('common', 'Windows-x64' if platform=='Windows-x64' else 'Linux-x64'):
        for name,digest in manifest[group].items():
            relative=Path(name)
            if relative.is_absolute() or '..' in relative.parts:
                raise RuntimeError('Invalid legacy package path')
            original=cache/group/relative
            if hashlib.sha256(original.read_bytes()).hexdigest()!=digest:
                raise RuntimeError('Legacy tool verification failed: '+str(original))
            target=Path(root)/relative;target.parent.mkdir(parents=True,exist_ok=True)
            shutil.copy2(original,target)
            if group=='Linux-x64' and relative.name in ('qbsp','vis','light','sdHLCSG','sdHLBSP','sdHLVIS','sdHLRAD'):
                target.chmod(0o755)
    shutil.copy2(cache/'manifest.json',Path(root)/'tools/legacy-games-manifest.json')
