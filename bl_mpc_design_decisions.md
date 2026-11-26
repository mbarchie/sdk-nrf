# Bootloader MPC API design decisions

| ID | Decision | Rationale |
|---|---|---|
| D01 | Lock-all API | Applies one final boot policy and locks unused overrides disabled. Per-override locking could leave protection incomplete or order-dependent. |
| D02 | No configuration getter | The module verifies read-back internally. A raw getter would expose hardware details and encourage unsupported reconfiguration. |
| D03 | SECATTR fixed to Secure | TrustZone attribution is outside this module's scope. TF-M configurations should let TF-M own MPC setup. |
| D04 | Public region capacity | `BL_MPC_REGION_COUNT_MAX` supports static array sizing and build-time validation. |
| D05 | Validate before writing | Predictable input errors and conflicting locks leave MPC00 unchanged. |
| D06 | Reject zero-region input | Prevents a successful call that configures no protected regions while still locking all overrides. |
| D07 | Fixed region-to-override mapping | Maps `regions[i]` to override `i`, making configuration deterministic and straightforward to verify. |
| D08 | Accept all RWX combinations | Restrictions such as prohibiting writable-and-executable regions belong to caller policy. |
| D09 | Reject repeated programming | Any enabled or locked software-configurable override is treated as evidence of a previous attempt and causes `-EACCES`. |
| D10 | No separate verification API | Protection is read back before success; callers do not inspect or reconfigure MPC state through this module. |
| D11 | No MEMACCERR control | Event handling introduces interrupt, NVIC, and handoff ownership outside protection configuration. |
| D12 | No public register pointer | MPC00 selection remains internal and allows family-specific backends later. |
| D13 | Read back after programming | Confirms that every override reached its requested state before control is handed off. |
| D14 | Fail-closed contract | Any error prohibits handoff, preventing execution with missing, conflicting, or partially applied protection. |
