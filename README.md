# MPK Tram System

To build the entire project (Slice files, system, passenger, and tram components):
```
make all
```

### Individual Components
Build specific components:
```
make build_slice    # Generate C++ from Slice files
make build_system   # Build the system component
make build_passenger # Build the passenger component
make build_tram     # Build the tram component
```

You can also build components WITHOUT slice
```
make comp    # Builds system, passenger, and tram components (but not Slice files)
```

After building, run the components in separate terminals:
```
./system
./passenger
./tram
```

Cleanup
Remove all generated files:
```
make clean
```
