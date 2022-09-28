# Device - API communication

## 1.0 Single device mode

First to implement. One document per device.

### Request new page chains

```mermaid
sequenceDiagram
    participant P as Print Server
    participant S as API Server
    participant D as Device


    %% ->>: synchronous
    %% -): asynchronous

    critical Device Registration
        D -) S: Register request
        Note over S,D: resolution, color depth
        S -) D: Register response
        Note over S,D: device index

        P -) S: New document printed

        critical Download: Start page
            S -) D: Show page signal
            Note over S,D: page index
            D ->> S: Page download request
            Note over S,D: page index
            S ->> D: Page download response
            Note over S,D: page image file
            D ->> D: Save & display page
        end

        opt Download: Remaining pages
            S -) D: Pages ready signal
            Note over S,D: num pages
            loop For each image
                D ->> S: Page download request
                Note over S,D: page index
                S ->> D: Page download response
                Note over S,D: page image file
                D ->> D: Save page
            end
        end

    end
```

## 2.0 Multi device mode

Second to implement. One document can be printed on multiple devices.

Like single device mode, but the API server can buffer multiple device registrations and page chains.

## 3.0 Paired devices

Last to implement. Multiple devices can be paired to print one document. Navigation between pages is synchronized.

### Page update

Called when a next/previous page button is clicked on the device.

```mermaid
sequenceDiagram
    participant S as API Server
    participant D as Device

    D -) S: Page update signal
    Note over S,D: page index

    S ->> S: Update page state
```

### Show page

Called when a device should show another page.

```mermaid
sequenceDiagram
    participant S as API Server
    participant D as Device

    S -) D: Show page signal
    Note over S,D: page index
```
