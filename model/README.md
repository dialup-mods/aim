This was a copy-paste from the sdk generator's model

Once the changes have been made here to support runtime access

It should be merged with the sdk generator's version

Some mechanism similar to Resolve should be done for resolving w/ engine access

Possibly split between SDK generated vs runtime, or combined and gated with a 'can access running engine' flag


And then ultimately put in Dial-Up core, or some other parenty place

Because it's useful to have these types for poking around in the live environment before you have the SDK generated, or are making changes to that and want to observe behavior before having to re-gen
