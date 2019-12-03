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

- Copy `nodevfee.exe` and `nodevfee.dll` to miner directory (in same directory with `EthDcrMiner64.exe`).
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

### WinDivert version:

- **3.6** - [divert3.6.zip](https://github.com/Demion/nodevfee/files/1820847/divert3.6.zip)
  * Output delay config.
  * Error message keyword / press any key after critical.
  * Add missing administrator rights request.
- **3.5** - [divert3.5.zip](https://github.com/Demion/nodevfee/files/1814706/divert3.5.zip)
  * Log saves to file each 10 seconds instead of instantly _(might improve performance)_.
  * Faster string (packet) pattern search _(might improve performance)_.
  * Tweaked WinDivert filter format _(might improve performance)_.
  * Tweaked / fixed log format.
  * Show / hide console feature.
  * Project files (VS2015) included with source code.
- **3.3** - [divert3.3.zip](https://github.com/Demion/nodevfee/files/1770819/divert3.3.zip)
- **3.2** - [divert3.2.zip](https://github.com/Demion/nodevfee/files/1768003/divert3.2.zip)
- **3.1** - [divert3.1.zip](https://github.com/Demion/nodevfee/files/1765753/divert3.1.zip)
- **3.0** - [divert3.zip](https://github.com/Demion/nodevfee/files/1755202/divert3.zip)
- **2.2** - [divert2.2.zip](https://github.com/Demion/nodevfee/files/1754700/divert2.2.zip)
- **2.1** - [divert2.1.zip](https://github.com/Demion/nodevfee/files/1754652/divert2.1.zip)
- **1.0** - [divert.zip](https://github.com/Demion/nodevfee/files/1754353/divert.zip)

`config.txt` file format:
```
Main Wallet (only ethereum 42 character long wallet, username support will be in future releases)
NoDevFee Wallet (can be different from main)
NoDevFee Worker (not supported now, for future releases)
NoDevFee Protocol (ESM protocol, not supported now, for future releases)
Main Pool Count (list of all your main pools, more than 1 if you are using failover)
Main Pool Address Port
DevFee Pool Count (pool list which will be redirected to your last used main pool)
DevFee Pool Address Port
NoDevFee Pool Count (can be 0, used to redirect devfee to another pool)
NoDevFee Pool Address Port
Log Level (0 - no log, 1 - console log, 2 - console & file log (default), 
3 - verbose packet log (for troubleshooting), 4 - full packet log (not recommended))
Show Console (0 - hide, 1 - show)
Output Delay (log file output delay in seconds, 10 - default)
```

_Can be run without_ `config.txt` _if you dont need redirection. Your Main Wallet and NoDevFee Wallet will be deduced from first authorization packet. If used config file should be strict format as mentioned above._

_No need to inject, no need any config files, can be located at any folder, just run `divert.exe`, run your miner as usual (without `nodevfee.exe`), `divert.exe` should keep running._

> **divert** version is based on **WinDivert** driver by **basil00** which is using _Windows Filtering Platform_.
> It is low level driver which intercepts and modifies network traffic. Should be less detectable and overall better approach as there is no direct interaction with miner (no dll injection, no memory modification (no winapi function hooks)).

**SSL not supported (both versions).** 

**_Project suspended (no plans to update)._**

**Use gpu memory timings (straps) on any miner** - cmdrv64 Close Driver Handle - https://github.com/Demion/cmdrv64

### Donation:

- ETH: 0xcb4effdeb46479caa0fef5f5e3569e4852f753a2
- BTC: 1H1zNLHNxqtMgVYJESF6PjPVq2h9tLW4xG

### Credits:

- minhook - The Minimalistic x86/x64 API Hooking Library for Windows (Tsuda Kageyu) http://www.codeproject.com/KB/winsdk/LibMinHook.aspx https://github.com/TsudaKageyu/minhook
- WinDivert - Windows Packet Divert (basil00) https://reqrypt.org/windivert.html https://github.com/basil00/Divert
