# Device - API communication

## Terms

- **Device**: Inkplate/Webbrowser (client)
- **API Server**: The server that handles the API requests
- **Print Server**: The server that handles the print requests
- **Page Chain**: An array of bitmaps, sorted by page number

## Initial registration

When a device boots/starts for the first time, it registers itself to the API Server. Registration information includes:

- uuid
- isBrowser
- screen info
  - resolution
    - width
    - height
  - color depth
  - dpi

The server ties this information to the id of the socket connection.

```mermaid
sequenceDiagram
    participant S as API Server
    participant D as Device


    %% ->>: synchronous
    %% -): asynchronous

    critical Device Registration
        D -) S: Register message
        Note over S,D: uuid, screen info, isBrowser

        S ->> S: Add to Registered Devices

        S -) D: Registered message
        Note over S,D: wasSuccessful
    end

    opt Device Unregistration

        %% Socket connection closes

        D -) S: Socket connection closes

        S ->> S: Remove from Registered Devices

    end
```

## Page chains

### Enqueue / Dequeue

A registered device can enqueue or dequeue itself to receive a page chain.
Enqueued devices receive a message once a new page chain is available.

```mermaid
sequenceDiagram
    participant S as API Server
    participant D as Device

    critical Enqueue
        D -) S: Enqueue message

        S ->> S: Add to Enqueued Devices

        S -) D: Update device index message
        Note over S,D: deviceIndex
    end

    opt Dequeue
        D -) S: Dequeue message

        S ->> S: Remove from Enqueued Devices

        S -) D: Update device index message
        Note over S,D: deviceIndex = -1
    end

```

### Requesting new page chains

A document can only be printed, if **at least one device is [enqueued](#enqueue--dequeue)**. Otherwise, the printing dialogue will return an error.

```mermaid
sequenceDiagram
    participant P as Print Server
    participant S as API Server
    participant D as Enqueued Device

    %% ->>: synchronous
    %% -): asynchronous

    P -) S: New document printed

    critical Download: Initial page
        S -) D: Show page message
        Note over S,D: page index (= device index)
        D ->> S: Page download GET request
        Note over S,D: page index
        S ->> D: Page download response
        Note over S,D: image blob
        D ->> D: Save & display page
    end

    opt Download: Remaining pages
        S -) D: Pages ready message
        Note over S,D: num pages
        loop For each image
            D ->> S: Page download request
            Note over S,D: page index
            S ->> D: Page download response
            Note over S,D: image blob
            D ->> D: Save page
        end
    end
```

## Device pairing

Multiple devices can be paired together to synchronize pagination.
The current implementation only supports prepending or appending devices to one another.

```mermaid
sequenceDiagram
    participant OD as Other Devices
    participant S as API Server
    participant D as Device

    critical Pairing
        D -) S: Pair message
        Note over S,D: doPrepend

        S ->> S: Add to Paired Devices

        S -) D: Update pairing index message
        Note over S,D: deviceIndex, numDevices
    end

    loop On every pagination
        D -) S: Page update message
        Note over S,D: newPageIndex

        S ->> OD: Show page message
        Note over S,OD: pageIndex
    end
```
