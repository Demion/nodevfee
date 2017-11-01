### Changelog:

- **v0.2.1** - [NoDevFee v0.2.1 x64](https://github.com/Demion/nodevfee/releases/download/v0.2.1/NoDevFee_v0.2.1_x64.zip)
  * External wallet config (nodevfeeWallet.txt).
- **v0.2** - [NoDevFee v0.2 x64](https://github.com/Demion/nodevfee/releases/download/v0.2/NoDevFee_v0.2_x64.zip)
  * Message box error reporting.
  * Automatic logging when nodevfeeLog.txt file exists.
  * Support eth-proxy Ethereum Stratum mode (-esm 0 default).
  * Support qtminer Ethereum Stratum mode (-esm 1).
  
### How to use:

- Copy nodevfee.exe and nodevfeeDll.dll to Claymore directory (in same directory with EthDcrMiner64.exe).
- Create bat file and use it nodevfee.exe EthDcrMiner64.exe YOUR_USUAL_PARAMETERS. Example:
`nodevfee.exe EthDcrMiner64.exe -epool eu1.ethermine.org:4444 -ewal 0xcb4effdeb46479caa0fef5f5e3569e4852f753a2.worker1 -epsw x -r 1`
- To make it work after miner restart add option -r 1 to bat file and create reboot.bat with exactly same parameters nodevfee.exe EthDcrMiner64.exe YOUR_USUAL_PARAMETERS -r 1
- To make it work with -allcoins 1 / -allpools 1 create nodevfeeWallet.txt file with your wallet address inside. *Note: devfee may still mine to different pool.*
- *Create nodevfeeLog.txt file in same directory to enable logging / delete to disable.*

### Donation:

- ETH: 0xcb4effdeb46479caa0fef5f5e3569e4852f753a2
- BTC: 1H1zNLHNxqtMgVYJESF6PjPVq2h9tLW4xG

### Credits:

- minhook - The Minimalistic x86/x64 API Hooking Library for Windows (Tsuda Kageyu) http://www.codeproject.com/KB/winsdk/LibMinHook.aspx https://github.com/TsudaKageyu/minhook
