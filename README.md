# Kronos
Kronos: A high precision virtual time system based on instruction counting

Full documentation can be found [here](https://kronoz.readthedocs.io/en/latest/index.html).
	
New in version 1.3: An update involing instruction counting based on Intel-PIN-TOOL is included in version 1.3
See examples/example_app_vt_experiment.py file where app_vt_tracer binaries are started (instead of the the
traditional tracer binaries). These app_vt_tracers rely on INTEL-PIN-TOOLs for instruction counting.

If you find Kronos useful in your work, please cite the following paper:

Babu, Vignesh, and David Nicol. "Precise Virtual Time Advancement for Network Emulation." Proceedings of the 2020 ACM SIGSIM Conference on Principles of Advanced Discrete Simulation. 2020.

Bibtex link [here](https://scholar.googleusercontent.com/scholar.bib?q=info:SLAZFhUreA8J:scholar.google.com/&output=citation&scisdr=CgXjqvZEEN-dn5CnHEQ:AAGBfm0AAAAAYEaiBERUP_wcBGnd5KcfOSS-oSuG031w&scisig=AAGBfm0AAAAAYEaiBPMjKaFAMP3P53yZOfWN2E9NtXGS&scisf=4&ct=citation&cd=-1&hl=en).
