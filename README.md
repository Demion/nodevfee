Changelog: 
- Message box error reporting.
- Automatic logging when nodevfeeLog.txt file exists.
- Support eth-proxy Ethereum Stratum mode (-esm 0 default).
- Support qtminer Ethereum Stratum mode (-esm 1).

How to use:
- Copy nodevfee.exe and nodevfeeDll.dll to Claymore directory (in same directory with EthDcrMiner64.exe).
- Create bat file and use it nodevfee.exe EthDcrMiner64.exe YOUR_USUAL_PARAMETERS. Example:
`nodevfee.exe EthDcrMiner64.exe -epool eu1.ethermine.org:4444 -ewal 0xcb4effdeb46479caa0fef5f5e3569e4852f753a2.worker1 -epsw x`
- *Create nodevfeeLog.txt file in same directory to enable logging.*

Download:
- [NoDevFee v0.2 x64](https://github.com/Demion/nodevfee/releases/download/v0.2/NoDevFee_v0.2_x64.zip)

Donation:
- ETH: 0xcb4effdeb46479caa0fef5f5e3569e4852f753a2
- BTC: 1H1zNLHNxqtMgVYJESF6PjPVq2h9tLW4xG

Credits:
- minhook - The Minimalistic x86/x64 API Hooking Library for Windows (Tsuda Kageyu) http://www.codeproject.com/KB/winsdk/LibMinHook.aspx https://github.com/TsudaKageyu/minhook
