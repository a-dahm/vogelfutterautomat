<?php
$upload_dir = "uploads";
$files = scandir($upload_dir);
$files = array_diff($files, array('.', '..'));
?>
<!DOCTYPE html>
<html>
<head>
    <style>
        * {
            margin: 0; box-sizing: border-box;
            font-family: "Consolas", monospace, serif;
        }
        #content {
            margin: 10px;
        }

        .container {
            display: flex;
            flex-flow: row wrap;
            justify-content: flex-start;
            gap: 10px;
            margin: 10px;
        }

        .separator {
            font-size: 20px;
            
        }

        .thumb {
            display: grid;
            grid-template:  "header" 20px
                            "content" 1fr
                            "footer" 30px;
            width: 300px;
            height: 225px;
            background-color: #00000022;
            border: 1px solid #00000055;
            border-radius: 5px;
            overflow: hidden;
        }

        .thumb>.header {
            grid-row: 1;
            grid-column: 1;
            z-index: 5;
            width: 100%;
            text-shadow: -1px -1px 0 #fff, 1px -1px 0 #fff, -1px 1px 0 #fff, 1px 1px 0 #fff;
        }

        .thumb > img, .thumb > audio {
            grid-row: 1 / span 3;
            grid-column: 1;
            align-self: center;
            justify-self: center;
            max-width: 300px;
            max-height: 225px;
        }

        .thumb > .footer {
            grid-row: 3;
            grid-column: 1;
            background-color: #00f;
            display: flex;
            justify-content: space-between;
        }

        .thumb > .header, .thumb > .footer {
            background-color: #00000011;
            text-align: center;
            font-weight: bold;
        }

        .thumb > img.content {
            cursor: zoom-in;
        }

        #modal {
            display: none;
            cursor: pointer;
            position: absolute;
            background-color: #00000077;
            width: 100%;
            height: 100%;
            flex-direction: column;
            align-items: center;
            justify-content: start;
            top: 0;
            z-index: 10;
        }
        #modal-image {
            max-width: 100%;
            max-height: 100%;
        }
        #modal-header {
            margin-top: 0px;
            margin-bottom: 5px;
            text-align: center;
            width: 100%;
            background-color: #000000aa;
            color: white;
        }
        .download-button {
            width: 30px;
            height: 30px;
            background-color: #000a;
            cursor: pointer;
        }
        .delete-button {
            width: 30px;
            height: 30px;
            background-color: #000a;
            cursor: pointer;
        }
    </style>
</head>
<body>
    <div id="content">
        
    </div>
    <div id="modal">
        <div id="modal-header"></div>
        <img id="modal-image" />
    </div>

    <script>
        const files = [<?php foreach($files as $file) { echo "\"$upload_dir/$file\", "; } ?>];
        const createThumb = (path) => {
            const fileName = path.split("/").pop();
            const element = document.createElement("div");
            element.classList.add("thumb");
            let contentControl = "";
            if(path.endsWith(".wav")) {
                contentControl = `
                    <audio controls class="content">
                        <source src="${path}" type="audio/wav">
                    </audio>
                `;
                
            } else {
                contentControl = `
                    <img src="${path}" data-path="${path}" class="content"  />
                `;
            }

            element.innerHTML = `
                    <span class="header">${fileName}</span>
                    ${contentControl}
                    <div class="footer">
                        <a title="Herunterladen" href="${path}" download="${path}" class="download-button">
                            <svg width="30px" height="30px" viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg"><path d="M5 12V18C5 18.5523 5.44772 19 6 19H18C18.5523 19 19 18.5523 19 18V12" stroke="#ffffff" stroke-linecap="round" stroke-linejoin="round"/><path d="M12 3L12 15M12 15L16 11M12 15L8 11" stroke="#ffffff" stroke-linecap="round" stroke-linejoin="round"/></svg>
                        </a>
                        <a title="Löschen" class="delete-button" data-path="${path}">
                            <svg width="30px" height="30px" viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
                            <path d="M4 7H20" stroke="#ff4444" stroke-linecap="round" stroke-linejoin="round"/>
                            <path d="M6 7V18C6 19.6569 7.34315 21 9 21H15C16.6569 21 18 19.6569 18 18V7" stroke="#ff4444" stroke-linecap="round" stroke-linejoin="round"/>
                            <path d="M9 5C9 3.89543 9.89543 3 11 3H13C14.1046 3 15 3.89543 15 5V7H9V5Z" stroke="#ff4444" stroke-linecap="round" stroke-linejoin="round"/>
                            </svg>
                        </a>
                    </div>
                `;
            
            return element;
        };

        function onThumbClick(e) {
            const modal = document.getElementById("modal");
            modal.style.display = "flex";
            document.getElementById("modal-image").src = e.target.src;
            document.getElementById("modal-header").innerText = e.target.dataset.path;
        }

        async function onDeleteClick(e) {
            let element = e.target;
            if(element.tagName !== "a")
                element = element.parentElement;
            const path = element.dataset.path;
            let confirmation = confirm(`Sind Sie sicher, dass Sie die Datei "${path}" löschen wollen?`);
            if(confirmation !== true) return;

            const response = await fetch("delete.php", {
                method: 'POST',
                body: path
            });
            if(response.status === 200) {
                const thumb = element.closest(".thumb");
                thumb.remove();
            } else {
                alert('Die Datei "${path}" konnte nicht gelöscht werden.');
            }
        }

        const content = document.getElementById("content");
        console.log(files);
        let lastDate = undefined;
        let lastContainer = undefined;
        for(let i = 0; i < files.length; i++) {
            const file = files[i];
            const match = /\/(\d{4}-\d{2}-\d{2})/.exec(file);
            let date = "";
            if(match) {
                date = match[1];
            }

            if(date !== lastDate) {
                if(lastDate !== undefined && lastContainer !== undefined) {
                    const separator = document.createElement("div");
                    separator.classList.add("separator");
                    separator.innerText = lastDate;
                    
                    content.appendChild(separator);
                    content.appendChild(lastContainer);
                }
                
                lastContainer = document.createElement("div");
                lastContainer.classList.add("container");
            }
            lastDate = date;
            lastContainer.appendChild(createThumb(file));
        }

        if(lastContainer) {
            const separator = document.createElement("div");
            separator.classList.add("separator");
            separator.innerText = lastDate;
            content.appendChild(separator);
            content.appendChild(lastContainer);
        }

        document.querySelectorAll(".thumb>img").forEach(element => element.addEventListener("click", onThumbClick));
        document.getElementById("modal").addEventListener("click", e => document.getElementById("modal").style.display = "none");
        document.querySelectorAll(".delete-button").forEach(element => element.addEventListener("click", onDeleteClick));
    </script>
</body>
