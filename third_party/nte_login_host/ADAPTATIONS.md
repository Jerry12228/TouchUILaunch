# TouchUILaunch NTE SDK host adaptation

Upstream: Hi3Helper.Plugin.NTE at commit 79863eeb2daa04d7558c54e2e6dc9a733102217e.
MIT license retained. UPSTREAM.json contains per-file SHA-256 values of the
pinned upstream Git blobs before local adaptations. upstream.patch records text
changes as a zero-context diff (apply with git apply --unidiff-zero against the
pinned source). Local text encoding/newlines are normalized separately.
The original refs clone is unchanged.

- Adds the exact installed Global 2.0.9 launcher/backend pair and statically
  verified public configuration RVA. Retains Global 2.0.8 and CN 2.0.8/2.0.9.
  Runtime registration still checks the selected version/application/owned PID.
  Static configuration/profile checks are not live authentication acceptance.
- Resolves CN SDK under NTELauncher instead of a stale GameLauncher.config path.
- Isolates storage, object names and DPAPI/Hello identifiers to TouchUILaunch.
  No existing official or Collapse account storage is imported.
- Disables official-launch preference, region file switching and native NTE.dll
  loading. Native CLI selects official relay vs independent host and region.
- Rewords product/storage notices for this component. Retains provider assets,
  style licenses and normal account consent/authentication behavior.
- Adds sanitized Global lifecycle phases and integration-only verification
  switches in games/nte/login_host/IntegrationVerification.cs. No credentials
  or SDK frames are logged. Invalid registration is reported rather than hidden.
- Builds a self-contained managed x64 apphost using .NET 10 and Avalonia 11.2.7.
  Assembly identity Nte.LoginHost is retained for Avalonia resource addresses.

Build/package: scripts/build.ps1 -NteLoginHost. Verify: scripts/test.ps1
-NteLoginHost. Normal native launch creates this exact host suspended and loads
our CreateProcessW bridge before it creates the game. The host owns SDK/login
callbacks until game exit. Default native tests need no .NET or game files.
