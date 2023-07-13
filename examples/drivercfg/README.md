# Driver Configuration Files

Some drivers within the EOM3K can be configured via JSON files. This is especially useful for dongles such as
the GPIO dongles where you can control scripted actions, and more efficient than running BASIC based scripts.

```json
{
    "match": {},
    "config": {},
    "functions": {},
    "events": {},
}
```

## `match`

This config key is used by a driver to identify valid config files, or to allow the EOM3K to identify unknown
devices and use a specific built-in driver. We'll figure this out later. For now, ensure the `match.driver` 
key includes the driver you want to load this config for.

If `match.vid`, `match.pid`, and `match.serial` all match the target device, it will be loaded automatically
at system startup / device connect.

## `config`

Generally this is up to the drivers loaded to consume. `config.displayName` is a convenient way to ID what
configuration is loaded for the connected device. Each driver will list this in their config requirements.

## `functions`

Here you can define functions. The key must start with an `@` and the value is an array of `Actions`. An 
`Action` can be a string indication a function name without parameters, or an object with keys for function
names, each having a value of an object containing the parameters. For example:

```json
[
    "@localFunctionCall",
    {
        "@localParamFunction": {
            "arg1": "foo",
        },
        "delay": {
            "ms": 1000,
            "then": [...more actions...]
        }
    }
]
```

If there is only one action, the array is optional. Note that for actions in the object form, this means you
can specify only an object containing multiple actions, so as long as the actions all have unique names. 
That's syntax sugar, baby!

Parameterless actions may receive NULL as a value. The following are the same:

```json
[
    "@ping",
    { "@ping": null }
]
```

## `events`

These are actions that can be performed in response to system events. Some events emit variables, which can be
used in the action sequence as well. Each event is a key which contains an actio nspecification same as 
functions.