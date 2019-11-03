at compile
-> link dma handler


once during instance creation
-> configure pm
-> configure clock
-> connect channel
-> create dma configuration

each transfer
-> pre-transfer callback
-> update dma configuration
-> start transfer

on callback rx done:
-> set complete flag, stop dma

on poll
-> if timeout. stop dma
-> if done... do stuff


