function AnimeRatings() {
}

var app = new AnimeRatings();
app.highlightTreshold  = 8;
app.visibilityTreshold = 6;


app.sendRequest = function(arg, callback) {
    chrome.extension.sendRequest(arg, function(response) {
        callback(response);
    });
};


app.log = function(message) {
    this.sendRequest(
        {action: "log", arg: message},
        function() {}
    );
};


app.getMALInfo = function(pageType, title, callback) {
    var linkInfo = {};
    linkInfo.title = title;
    linkInfo.pageType = pageType;
    this.sendRequest(
        {action: "getMalInfo", arg: linkInfo},
        function(linkInfo) {
            callback(linkInfo);
        }
    );
};


app.getMWPages = function() {
    var divs = document.getElementsByTagName("div");
    for (var i = 0; i < divs.length; ++i) {
        if (divs[i].id == "mw-pages") {
            return divs[i];
        }
    }
    throw "MW Pages not found";
};


app.debugLink = "";
app.excludeLink = "";


app.getLinks = function() {
    var result = [];
    try {
        var lis = this.getMWPages().getElementsByTagName("li");
        for (var i = 0; i < lis.length; ++i) {
            var links = lis[i].getElementsByTagName("a");
            if (links.length > 0) {
                var linkNode = links[0];
                if (linkNode.title.search(app.debugLink) !== -1 &&
                    linkNode.title.search(app.excludeLink === -1)) {
                    result.push(linkNode);
                }
            }
        }
    }
    catch (exc) {
        // MW pages not found on this page.
        // This can happen for summary pages like "Category:Anime_of_the_2000s"
    }
    return result;
};


app.encodeResult = function(title) {

    var result = title;
    var keys = [];
    var values = [];
    keys.push(/&Atilde;&copy;/g); values.push("&eacute;");
    keys.push(/&Atilde;&uml;/g); values.push("&egrave;");
    keys.push(/&Atilde;&ordf;/g); values.push("&ecirc;");
    keys.push(/&Atilde;&laquo;/g); values.push("&euml;");
    keys.push(/&Atilde;&nbsp;/g); values.push("&agrave;");
    keys.push(/&Atilde;&curren;/g); values.push("&auml;");
    keys.push(/&Atilde;&cent;/g); values.push("&acirc;");
    keys.push(/&Atilde;&sup1;/g); values.push("&ugrave;");
    keys.push(/&Atilde;&raquo;/g); values.push("&ucirc;");
    keys.push(/&Atilde;&frac14;/g); values.push("&uuml;");
    keys.push(/&Atilde;&acute;/g); values.push("&ocirc;");
    keys.push(/&Atilde;&para;/g); values.push("&ouml;");
    keys.push(/&Atilde;&reg;/g); values.push("&icirc;");
    keys.push(/&Atilde;&macr;/g); values.push("&iuml;");
    keys.push(/&Atilde;&sect;/g); values.push("&ccedil;");
    keys.push(/&amp;/g); values.push("&amp;");

    keys.push(/&nbsp;/g); values.push(" ");
    keys.push(/&auml;/g); values.push("ä");
    keys.push(/&uuml;/g); values.push("ü");

    for (var i = 0; i < keys.length; ++i) {
        result = result.replace(keys[i], values[i]);
    }
    return result;
};


app.htmlDecode = function(input) {
    var e = document.createElement('div');
    e.innerHTML = input;
    return e.childNodes.length === 0 ? "" : e.childNodes[0].nodeValue;
};


app.findAndReplace = function(input, mapping) {
    var result = input;
    for (var key in mapping) {
        var value = mapping[key];
        var count = 0;
        while (result.search(key) !== -1) {
            result = result.replace(key, value);
            if (count++ > 20) {
                this.log("Problematic replacement key: " + key);
                break;
            }
        }
    }
    return result;
};

/**
 * For some reason the unicode in the xml response
 * is wrong. Either I am doing something wrong, or
 * MAL wrongly encodes the response.
 *
 * This code is a workaround that provides fixes
 * for common cases.
 */
app.fixUnicode = function(input) {
    var result = input;

    // First apply a mapping of composed keys
    result = this.findAndReplace(result, {
        "&acirc;��&acirc;��&acirc;��"   : "☆☆☆",
        "&Atilde;�&Atilde;�&Atilde;�"   :"xxx",
        "&acirc;��"                     : "-"
    });

    // Then map remaining individual keys
    result = this.findAndReplace(result, {
        "&Atilde;&copy;"    : "é",
        "&acirc;�&ordf;"    : "♪",
        "&acirc;�"          : "†",
        "&Aring;�"          : "ō"
    });
    return result;
};


app.getYear = function() {
    var components = document.URL.split("_");
    return components[components.length - 1];
};


/**
 * @pattern  String  "{Year} {Title} {Score}"
 */
app.addEntryToDOM = function(parent, entry, pattern) {

    parent = parent.create("li");

    parent.setAttribute("score", entry.score);
    app.malLinks.push(parent);

    parent = parent.create("a");
    parent.setAttribute("href", "http://myanimelist.net/" + this.getPageType() + "/" + entry.id);

    var result = pattern;
    result = result.replace("{BeginYear}",  parseInt(entry.start_date.split("-")[0], 10));
    result = result.replace("{EndYear}",  parseInt(entry.end_date.split("-")[0], 10));
    result = result.replace("{Title}", entry.title);
    result = result.replace("{Score}", entry.score !== "0.00" ? entry.score : "no rating");
    parent.createText(this.htmlDecode(this.fixUnicode(this.encodeResult(result))));

    app.updateScore();
};


app.informFailure = function(node, linkItem) {
    var reason = (linkItem.reason === undefined ? "No results returned." : linkItem.reason);
    var parent = node.parentNode;
    parent = parent.createEntryList().create("li");
    parent.createText(reason);
};


app.addEntriesToDOM = function(node, linkItem) {
    var parent = node.parentNode;


    var entries = linkItem.entries;
    app.sortEntries(entries);

    if (parent.getElementsByTagName("ul").length === 0) {
        parent = parent.createEntryList();
        parent.style.listStyle = "square outside none";
    }

    for (var i = 0; i < entries.length; ++i) {
        try {
            var entry = entries[i];

            // Skip specials
            if (entry.type === "Special" || entry.type == "One Shot") {
                continue;
            }

            // Skip if year doesn't match
            var startYear = parseInt(entry.start_date.split("-")[0], 10);
            var end_year = parseInt(entry.end_date.split("-")[0], 10);
            var year = this.getYear();
            if (startYear > year || (end_year !== 0 && end_year < year)) {
                if (entry.title.search("Hunter") !== -1) {
                    this.log(year + " is outside of [" + startYear + ", " + end_year + "]");
                    this.log("entry.end_date: " + entry.end_date);
                }
                parent.setAttribute("private_year_is_wrong", true);
                continue;
            }

            // Insert title entry
            this.addEntryToDOM(parent,
                               entry,
                               "{Title} ({Score})");
        }
        catch (exc) {
            app.log(exc);
        }
    }

    if (parent.hasAttribute("private_year_is_wrong") &&
        parent.getElementsByTagName("li").length === 0) {
        app.addMissingStuff(parent, entries);
    }
};


app.addMissingStuff = function(listElement, entries) {

    if (entries.length === 0) {
        return;
    }

    var closest_entry = entries[0];
    closest_entry.difference = Math.abs(this.getYear() - parseInt(closest_entry.start_date.split("-")[0], 10));

    for (var i = 1; i < entries.length; ++i) {
        try {
            var entry = entries[i];
            entry.difference = Math.abs(this.getYear() - parseInt(entry.start_date.split("-")[0], 10));
            if (entry.difference < closest_entry.difference) {
                closest_entry = entry;
            }
        }
        catch (exc) {
            app.log(exc);
        }
    }

    // If the closest match is more than one year difference the we don't consider it.
    if (closest_entry.difference != 1) {
        return;
    }


    listElement.style.listStyle = "none";

    var parent = listElement.create("li");
    parent.createText("No MAL title found from " + this.getYear() + ". Closest match:");

    parent = parent.create("ul");
    parent.style.listStyle = "square outside none";

    // Insert title entry
    this.addEntryToDOM(parent, closest_entry, "{BeginYear}: {Title} ({Score})");
};


/**
 * Workaround: improves search results
 */
app.improveTitle = function(title) {

    var result = title;

    // If the title contains a fragment, then only search for the mapped fragment.
    var tmapping = {
        "11eyes: Tsumi to Batsu to Aganai no Shōjo" : "11eyes",
        "A Channel": "A-Channel",
        "Aki Sora" : "Aki-Sora",
        "Ano Hi Mita Hana": "Ano Hi Mita Hana",
        "Doraemon" : "Doraemon",
        "Hayate the Combat Butler!" : "Hayate",
        "Heaven's Lost Property" : "Heaven's Lost Property",
        "Infinite Stratos" : "Infinite Stratos",
        "Mawaru-Penguindrum" : "Mawaru Penguindrum",
        "Mazinkaizer" : "Mazinkaiser",
        "Negima! Magister Negi Magi" : "Negima",
        "Oh My Goddess" : "My Goddess",
        "Poppy Hill" : "Kokurikozaka Kara",
        "Koishinasai" : "Koi Shinasai",
        "Sengoku Paradise" : "Kiwami",
        "Shin Megami Tensei: Persona 4" : "Shin Megami",
        "Yondemasuyo" : "Yondemasu yo"
    };

    for (var tkey in tmapping) {
        if (result.search(tkey) !== -1) {
            result = tmapping[tkey];
            break;
        }
    }

    // Improve fragments
    var fmapping = {
        "×" : "x",
        "ō" : "ou",
        "Ō" : "Ou",
        "ū" : "uu",
        "ä" : "a",
        "Ä" : "A",
        "½" : "1/2",
        "&amp;" : "&",
        "(anime)": "",
        "(film)": "",
        "(manga)": "",
        "(movie)": "",
        "(visual novel)" : "",
        "(novel)": "",
        "(video game)": "",
        "(novel series)" : "",
        "(Japanese series)" : "",
        "(TV series)" : ""
    };

    for (var fkey in fmapping) {
        var fragment = fmapping[fkey];
        var count = 0;
        while (result.search(fkey) !== -1 && count < 10) {
            result = result.replace(fkey, fragment);
            count++;
        }
        if (count >= 10) {
            this.log("Problematic replacement: " + result + ", with key: " + fkey);
        }
    }

    // Trim
    result = result.replace(/^\s+|\s+$/g, "");

    return result;
};


Element.prototype.toggle = function() {
	if (this.style.display != 'none' ) {
		this.style.display = 'none';
	}
	else {
		this.style.display = '';
	}
};


Element.prototype.createText = function(text) {
    this.appendChild(document.createTextNode(text));
};


Element.prototype.createNBSP = function() {
    this.appendChild(document.createTextNode("\u00a0"));
};


Element.prototype.create = function(tagNamePath) {
    var result = this;
    var tagNames = tagNamePath.split("/");
    for (var i = 0; i < tagNames.length; ++i) {
        var child = document.createElement(tagNames[i]);
        result.appendChild(child);
        result = child;
    }
    return result;
};


Element.prototype.createEntryList = function() {
    var result = this.create("small/ul");
    result.style.listStyle = "square outside none";
    return result;
};

app.getNext = function() {
    if (app.links.length === 0) {
        app.updateScore();
        return;
    }

    var linkNode = app.links.pop();

    var title = app.improveTitle(linkNode.title);
    if (app.linkNodes[title] === undefined) {
        app.linkNodes[title] = linkNode;
        this.getMALInfo(this.getPageType(), title, function(linkInfo) {
            var node = app.linkNodes[title];
            if (linkInfo.success === true) {
                app.addEntriesToDOM(node, linkInfo);
            }
            else {
                app.informFailure(node, linkInfo);
            }
            app.getNext();
        });
    }
};


app.isYearList = function() {
    return app.getPageType() === "anime" ||
           app.getPageType() === "manga";
};

app.getAnimeTitleFromPage = function() {
    var headings = document.getElementsByTagName("h1");
    if (headings.length === 0) {
        throw "H1 not found in page";
    }

    if (typeof(headings[0].childNodes[0].nodeValue) === "string") {
        return headings[0].childNodes[0].nodeValue;
    }

    if (typeof(headings[0].childNodes[0].innerHTML) === "string") {
        return headings[0].childNodes[0].innerHTML;
    }

    throw "Failed to get Wikipedia page title";
};


/**
 * Get the link to www.animenewsnetwork.com.
 * Returns null if not found.
 */
app.getANNLink = function() {
    var links = document.getElementsByTagName("a");
    for (var i = 0; i < links.length; ++i) {
        var link = links[i];
        var href = link.href;
        if (href.search("http://www.animenewsnetwork.com/encyclopedia") !== -1) {
            return link;
        }
    }
    return null;
};


app.getPageTypeFromInfoBox = function() {
    var foundAnime = false;
    var foundManga = false;
    var tables = document.getElementsByTagName("table");
    for (var i = 0; i < tables.length; ++i) {
        var table = tables[i];
        if (table.getAttribute("class") === "infobox") {
            var tds = table.getElementsByTagName("td");
            for (var j = 0; j < tds.length; ++j) {
                var td = tds[j];
                if (td.childNodes.length === 0) {
                    continue;
                }

                if (td.childNodes[0].nodeValue === null) {
                    continue;
                }

                var value = td.childNodes[0].nodeValue;

                if (value.search(/TV anime/) !== -1 ||
                    value.search(/Original video animation/) !== -1) {
                    foundAnime = true;
                }

                if (value.search(/Manga/) !== -1) {
                    foundManga = true;
                }
            }
        }
    }

    if (foundAnime && foundManga) {
        return "anime|manga";
    }

    if (foundAnime) {
        return "anime";
    }

    if (foundManga) {
        return "manga";
    }

    return "default";
};


app.getPageType = function() {
    if (document.URL.search("Category:Anime_of_") !== -1) {
        return "anime";
    }
    else if (document.URL.search("Category:Manga_of_") !== -1) {
        return "manga";
    }
    return "default";
};


app.insertRatingsIntoList = function() {
    var numSimulReq = 5;
    for (var i = 0; i < numSimulReq; ++i) {
        app.getNext(app.links);
    }
};


app.insertRatingsIntoPage = function() {
    var title = app.improveTitle(app.getAnimeTitleFromPage());
    this.getMALInfo("anime", title, function(linkInfo) {
        app.linkInfo = linkInfo;
        app.getMALInfo("manga", title, function(linkInfo) {
            app.linkInfo.success = app.linkInfo || linkInfo.success;
            if (!app.linkInfo.success) {
                app.log("Failed to get neither anime nor manga info for: " + title);
                return;
            }
            for (var i = 0; i < linkInfo.entries.length; ++i) {
                app.linkInfo.entries.push(linkInfo.entries[i]);
            }
            app.addRatingIntoAnimePageDOM(app.linkInfo);
        });
    });
};


app.getFirstChildByTagName = function(node, tagName) {
    for (var i = 0; i < node.childNodes.length; ++i) {
        var childNode = node.childNodes[i];
        if (childNode.tagName === tagName) {
            return childNode;
        }
    }
    return null;
};


app.sortEntries = function(entries) {
    entries.sort(function(lhs, rhs) {
        if (lhs.start_date < rhs.start_date) {
            return -1;
        }
        else if (lhs.start_date == rhs.start_date) {
            return 0;
        }
        return 1;
    });
};


app.addRatingIntoAnimePageDOM = function(linkInfo) {

    // Don't show the the info box if on entries were found.
    if (linkInfo.entries.length === 0) {
        return;
    }
    
    app.sortEntries(linkInfo.entries);

    var firstParagraph = app.getFirstChildByTagName(document.getElementById("bodyContent"), "P");

    var node = firstParagraph.parentNode.insertBefore(document.createElement("div"), firstParagraph);
    node = node.create("table");
    node.setAttribute("style", "margin: 0.5em 0 0.5em 1em; padding: 0.2em;");
    node.setAttribute("class", "toc");

    var td = node.create("tr/td");
    td.setAttribute("style", "text-align: center;");
    var table_title = td.create("a");
    table_title.href = "http://chrome.google.com/webstore/detail/aneeljmnclggefejjbbbbploekjpfejc";
    table_title.create("strong").createText("Anime Ratings");

    td.createNBSP();
    td.createNBSP();
    td = td.create("span");
    td.setAttribute("class", "toctoggle");
    td.createText("[");
    app.hideButton = td.create("a");
    app.hideButton.href = '#';
    td.createText("]");

    app.hideButton.innerText = "hide";

    var table = node.create("table");

    app.malInfoBox = table;

    app.hideButton.onclick = function() {
        app.malInfoBox.toggle();
        app.hideButton.innerText = app.malInfoBox.style.display === "none" ? "show" : "hide";
    };
    
    tr = table.create("tr");

    tr.setAttribute("style", "text-align: center;");
    td = tr.create("td").create("b");
    td.createText("Year");

    td = tr.create("td").create("b");
    td.createText("Title");

    td = tr.create("td").create("b");
    td.createText("Type");

    td = tr.create("td");
    td.create("b").createText("Rating");

    for (var i = 0; i < linkInfo.entries.length; ++i) {

        var entry = linkInfo.entries[i];

        // Skip specials
        if (entry.type === "Special" || entry.type == "One Shot") {
            continue;
        }

        // Year
        var startYear = parseInt(entry.start_date.split("-")[0], 10);

        var endYear = parseInt(entry.end_date.split("-")[0], 10);
        if (endYear === 0) {
            endYear = "ongoing";
        }

        tr = table.create("tr");
        var td_year = tr.create("td");
        td_year.setAttribute("style", "text-align: center;");
        var year = startYear;

        if (startYear === 0) {
            year = "Unknown";
        }
        else if (startYear !== endYear) {
            year += " - " + endYear;
        }
        td_year.createText(year);

        // Title
        var td_title = tr.create("td/i/a");
        td_title.setAttribute("style", "text-align: left;");
        td_title.setAttribute("href", "http://myanimelist.net/" + entry.pageType + "/" + entry.id);
        td_title.createText(this.htmlDecode(this.fixUnicode(this.encodeResult(entry.title))));

        // Type
        var td_type = tr.create("td");
        td_type.setAttribute("style", "text-align: center;");
        td_type.createText(entry.type);

        // Score
        var td_score = tr.create("td");
        td_score.setAttribute("style", "text-align: center;");
        if (parseFloat(entry.score, 10) >= 8) {
            td_score = td_score.create("strong");
        }
        td_score.createText(entry.score !== "0.00" ? entry.score : "(none)");

    }
};


app.insertSettingsBox = function() {
    var table = document.createElement("table");
    table.className = "toc";
    table.setAttribute("style", "float:none;");
    
    var mwPages = app.getMWPages();
    mwPages.insertBefore(table, mwPages.firstChild.nextSibling.nextSibling.nextSibling.nextSibling);

    table = table.create("tbody");

    var tr = table.create("tr");
    tr.setAttribute("style", "font-weight:bold;");
    
    var th = tr.create("th");
    th.setAttribute("style", "text-align: center;");

    var title_link = th.create("a");
    title_link.create("strong").createText("Anime Ratings");
    title_link.href = "http://chrome.google.com/webstore/detail/aneeljmnclggefejjbbbbploekjpfejc";

    table = table.create("table");

    tr = table.create("tr");
    var td = tr.create("td");
    td.setAttribute("style", "vertical-align:middle;");
    td.createText("Visibility treshold: ");

    var spinButtonWidth = "40px";

    td = tr.create("td");
    app.visibilitySpinButton = td.create("input");
    app.visibilitySpinButton.name = "VisibilityTreshold";
    app.visibilitySpinButton.type = "number";
    app.visibilitySpinButton.min = "0";
    app.visibilitySpinButton.max = "10";
    app.visibilitySpinButton.step = "0.1";
    app.visibilitySpinButton.setAttribute("style", "width:" + spinButtonWidth + ";");

    var visibilityTreshold = localStorage["visibilityTreshold"];
    app.setVisibilityTreshold((visibilityTreshold !== undefined) ? visibilityTreshold : app.visibilityTreshold);

    tr = table.create("tr");
    td = tr.create("td");
    td.setAttribute("style", "vertical-align:middle;");
    td.createText("Highlight treshold: ");

    td = tr.create("td");
    app.highlightSpinButton = td.create("input");
    app.highlightSpinButton.name = "HighlightTreshold";
    app.highlightSpinButton.type = "number";
    app.highlightSpinButton.min = "0";
    app.highlightSpinButton.max = "10";
    app.highlightSpinButton.step = "0.1";
    app.highlightSpinButton.setAttribute("style", "width:" + spinButtonWidth + ";");

    var highlightTreshold = localStorage["highlightTreshold"];
    app.setHighlightTreshold((highlightTreshold !== undefined) ? highlightTreshold : app.highlightTreshold);

};


app.string2float = function(value, defaultValue) {
    var result = parseFloat(value, 10);
    if (isNaN(result)) {
        result = defaultValue;
    }
    return result;
};


app.getVisibilityTreshold = function() {
    var result = app.string2float(app.visibilitySpinButton.value, app.visibilityTreshold);
    if (result < 0 || result > 10) {
        result = app.visibilityTreshold;
        app.visibilitySpinButton.value = result;
    }
    return result;
};


app.getHighlightTreshold = function() {
    var result = app.string2float(app.highlightSpinButton.value, app.highlightTreshold);
    if (result < 0 || result > 10) {
        result = app.highlightTreshold;
        app.highlightSpinButton.value = result;
    }
    return result;
};


app.setVisibilityTreshold = function(visibilityTreshold) {
    app.visibilitySpinButton.value = visibilityTreshold;
};


app.setHighlightTreshold = function(highlightTreshold) {
    app.highlightSpinButton.value = highlightTreshold;
};


app.isVisibilityTresholdChanged = function() {
    if (app.visibilityTreshold !== app.getVisibilityTreshold()) {
        app.visibilityTreshold = app.getVisibilityTreshold();
        localStorage["visibilityTreshold"] = app.visibilityTreshold;
        return true;
    }

    return false;
};


app.isHighlightTresholdChanged = function() {
    if (app.highlightTreshold !== app.getHighlightTreshold()) {
        app.highlightTreshold = app.getHighlightTreshold();
        localStorage["highlightTreshold"] = app.highlightTreshold;
        return true;
    }

    return false;
};


app.getMALLinkScore = function(malLink) {
  return parseFloat(malLink.getAttribute("score"), 10);
};


app.updateScoreLoopIsRunning = false;
app.forceUpdateScore = false;


app.updateScore = function() {
    app.forceUpdateScore = true;
    
    if (app.updateScoreLoopIsRunning === false) {
        app.updateScoreLoopIsRunning = true;
        app.updateScoreImpl();
    }
};


app.updateScoreImpl = function() {
    if (app.forceUpdateScore === true ||
        app.isHighlightTresholdChanged() ||
        app.isVisibilityTresholdChanged())
    {
        app.forceUpdateScore = false;
        
        for (var i = 0; i < app.malLinks.length; ++i) {
            var malLink = app.malLinks[i];
            var linkScore = app.getMALLinkScore(malLink);

            if (linkScore >= app.getHighlightTreshold()) {
                malLink.firstChild.style.fontWeight = "bold";
                malLink.firstChild.style.backgroundColor = "yellow";
            }
            else {
                malLink.firstChild.style.fontWeight = "normal";
                malLink.firstChild.style.backgroundColor = "inherit";
            }

            if (linkScore >= app.getVisibilityTreshold()) {
                malLink.style.display = "list-item";
            }
            else {
                malLink.style.display = "none";
            }
        }
    }
    window.setTimeout(app.updateScoreImpl, 500);
};


//
// Application Entry Point
//
try {

app.malLinks = [];
app.linkNodes = {};
app.links = app.getLinks().reverse();

if (app.isYearList()) {
    app.insertSettingsBox();
    app.insertRatingsIntoList();
    app.updateScore();
}
else {
    app.pageType = app.getPageTypeFromInfoBox();
    if (app.pageType === "default" && app.getANNLink() !== null) {
        app.pageType = "anime";
    }
    if (app.pageType.search("anime") !== -1 || app.pageType.search("manga") !== -1) {
        app.insertRatingsIntoPage();
    }
}

} catch (exc) {
    app.log("Exception caught: " + exc.toString());
}
