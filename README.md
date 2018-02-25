### Changelog:

- **v0.2.6b** - [NoDevFee v0.2.6b x64 Experimental](https://github.com/Demion/nodevfee/releases/download/v0.2.6b/NoDevFee_v0.2.6b_x64.zip)
  * Executable / config file rename possibility.
  * Copy packet data buffer before modification.
  * Improve detection already injected DLL.
- **v0.2.5b** - [NoDevFee v0.2.5b x64 Experimental](https://github.com/Demion/nodevfee/releases/download/v0.2.5b/NoDevFee_v0.2.5b_x64.zip)
  * Fix host name resolution (improve pool connection redirection).
- **v0.2.4b** - [NoDevFee v0.2.4b x64 Experimental](https://github.com/Demion/nodevfee/releases/download/v0.2.4b/NoDevFee_v0.2.4b_x64.zip)
  * Improve interception methods (3rd party miners).
- **v0.2.3b** - [NoDevFee v0.2.3b x64 Experimental](https://github.com/Demion/nodevfee/releases/download/v0.2.3b/NoDevFee_v0.2.3b_x64.zip)
  * Process DLL injector (`nodevfeeInject.txt`).
- **v0.2.2b** - [NoDevFee v0.2.2b x64 Experimental](https://github.com/Demion/nodevfee/releases/download/v0.2.2b/NoDevFee_v0.2.2b_x64.zip)
  * Pool redirection (`nodevfeePools.txt`).
  * Support `-eworker`.
- **v0.2.1** - [NoDevFee v0.2.1 x64](https://github.com/Demion/nodevfee/releases/download/v0.2.1/NoDevFee_v0.2.1_x64.zip)
  * External wallet config (`nodevfeeWallet.txt`).
- **v0.2** - [NoDevFee v0.2 x64](https://github.com/Demion/nodevfee/releases/download/v0.2/NoDevFee_v0.2_x64.zip)
  * Message box error reporting.
  * Automatic logging when `nodevfeeLog.txt` file exists.
  * Support eth-proxy Ethereum Stratum mode (`-esm 0` default).
  * Support qtminer Ethereum Stratum mode (`-esm 1`).
  
### How to use:

- Copy `nodevfee.exe` and `nodevfee.dll` to Claymore directory (in same directory with `EthDcrMiner64.exe`).
- Create bat file and use it `nodevfee.exe EthDcrMiner64.exe YOUR_USUAL_PARAMETERS`.

Example:
```
nodevfee.exe EthDcrMiner64.exe -epool eu1.ethermine.org:4444 -ewal 0xcb4effdeb46479caa0fef5f5e3569e4852f753a2.worker1 -epsw x -r 1
```
- To make it work after miner restart add option `-r 1` to bat file and create reboot.bat with exactly same parameters `nodevfee.exe EthDcrMiner64.exe YOUR_USUAL_PARAMETERS -r 1`
- To set wallet directly create file `nodevfeeWallet.txt` with your wallet address inside. *Note: might be needed to work with* `-allcoins` / `-allpools`.
- Create `nodevfeeLog.txt` file in same directory to enable logging / delete to disable.
- To work with 3rd party miners create `nodevfeeInject.txt` with your miner file name inside; run `nodevfee.exe` without parameters; run your miner as usual (without `nodevfee.exe` before miner). *Note: `nodevfee.exe` should keep running; `nodevfee.exe` `nodevfee.dll` and all config files should be in same directory as your miner.*
- Executable / config file rename is possible. Format: `example.exe` `example.dll` `exampleWallet.txt` `examplePools.txt` `exampleInject.txt` `exampleLog.txt`. *Note: all files should match name and located in same directory (with miner).*
- To redirect devfee pools to your main pool create file `nodevfeePools.txt`. 

Example (redirecting pools to `eu1.ethermine.org:4444`): https://pastebin.com/bWd1QAAe

Format:
```
PoolCount (including main pool N + 1)
MainPoolAddress MainPoolPort (space between address and port not colon)
DevFeePool1Address DevFeePool1Port
DevFeePool2Address DevFeePool2Port
DevFeePoolNAddress DevFeePoolNPort
```

### Donation:

- ETH: 0xcb4effdeb46479caa0fef5f5e3569e4852f753a2
- BTC: 1H1zNLHNxqtMgVYJESF6PjPVq2h9tLW4xG

### Credits:

- minhook - The Minimalistic x86/x64 API Hooking Library for Windows (Tsuda Kageyu) http://www.codeproject.com/KB/winsdk/LibMinHook.aspx https://github.com/TsudaKageyu/minhook
