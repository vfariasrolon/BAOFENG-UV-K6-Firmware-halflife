import dotnetfile

pe = dotnetfile.DotNetPE('BFK6_Bootloader.exe')
pe.parse_all()

# Get User Strings stream
user_strings = pe.get_user_stream_strings()
print(f"Found {len(user_strings)} strings in the User String (#US) stream.")

print("\nListing interesting User Strings:")
for i, s in enumerate(user_strings):
    # Print strings that are longer than 2 characters
    if len(s) > 2:
        # Check if it has hex characters or looks like a command/response
        print(f" - [{i}]: '{s}'")
