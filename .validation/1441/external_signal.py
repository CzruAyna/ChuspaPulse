import selectors
import signal
import subprocess
import sys

process = subprocess.Popen([sys.argv[1], 'external'], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
try:
    with selectors.DefaultSelector() as selector:
        selector.register(process.stdout, selectors.EVENT_READ)
        if not selector.select(timeout=5):
            raise RuntimeError('The probe did not become ready')
        if process.stdout.readline().strip() != 'READY':
            raise RuntimeError('The probe did not announce readiness')
    process.send_signal(getattr(signal, sys.argv[2]))
    stdout, stderr = process.communicate(timeout=5)
    if process.returncode != 0 or stdout.strip() != 'STOPPED_CLEANLY':
        raise RuntimeError(f'Unclean shutdown: code={process.returncode}, stdout={stdout!r}, stderr={stderr!r}')
    print(f'{sys.argv[2]} received from another process; normal exit code 0')
finally:
    if process.poll() is None:
        process.kill()
        process.communicate(timeout=5)
