function AnimeRatings() {
}

try {

var animeRatings = new AnimeRatings();


animeRatings.sendRequest = function(arg, callback) {
    chrome.extension.sendRequest(arg, function(response) {
        callback(response);
    });
};


animeRatings.log = function(message) {
    this.sendRequest(
        {action: "log", arg: message},
        function() {}
    );
};


animeRatings.getPageType = function() {
    if (document.URL.search("Anime") !== -1) {
        return "anime";
    }
    else if (document.URL.search("Manga") !== -1) {
        return "manga";
    }
    else {
        throw "Invalid URL: " + document.URL;
    }
};


animeRatings.getMALInfo = function(title, callback) {
    var linkInfo = {};
    linkInfo.title = title;
    linkInfo.pageType = this.getPageType();
    this.sendRequest(
        {action: "getMalInfo", arg: linkInfo},
        function(linkInfo) {
            callback(linkInfo);
        }
    );
};


animeRatings.getMWPages = function() {
    var divs = document.getElementsByTagName("div");
    for (var i = 0; i < divs.length; ++i) {
        if (divs[i].id == "mw-pages") {
            return divs[i];
        }
    }
    throw "MW Pages not found";
};


animeRatings.debugLink = "";
animeRatings.excludeLink = "";


animeRatings.getLinks = function() {
    var result = [];
    try {
        var lis = this.getMWPages().getElementsByTagName("li");
        for (var i = 0; i < lis.length; ++i) {
            var links = lis[i].getElementsByTagName("a");
            if (links.length > 0) {
                var linkNode = links[0];
                if (linkNode.title.search(animeRatings.debugLink) !== -1 &&
                    linkNode.title.search(animeRatings.excludeLink === -1)) {
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


animeRatings.encodeResult = function(title) {

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


animeRatings.htmlDecode = function(input) {
    var e = document.createElement('div');
    e.innerHTML = input;
    return e.childNodes.length === 0 ? "" : e.childNodes[0].nodeValue;
};


animeRatings.findAndReplace = function(input, mapping) {
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
animeRatings.fixUnicode = function(input) {
    var result = input;

    // First apply a mapping of composed keys
    result = this.findAndReplace(result, {
        "&acirc;��&acirc;��&acirc;��"	: "☆☆☆",
        "&Atilde;�&Atilde;�&Atilde;�"		:"xxx",
        "&acirc;��"						: "-"
    });

    // Then map remaining individual keys
    result = this.findAndReplace(result, {
        "&Atilde;&copy;"	: "é",
        "&acirc;�&ordf;"	: "♪",
        "&acirc;�"			: "†",
        "&Aring;�"			: "ō"
    });
    return result;
};


animeRatings.getYear = function() {
    var components = document.URL.split("_");
    return components[components.length - 1];
};


/**
 * @pattern  String  "{Year} {Title} {Score}"
 */
animeRatings.addEntryToDOM = function(listItem, entry, pattern) {

    var parent = listItem.create("a");
    parent.setAttribute("href", "http://myanimelist.net/" + this.getPageType() + "/" + entry.id);

    if (parseFloat(entry.score, 10) >= 8) {
        parent = parent.create("strong");
        parent.setAttribute("style", "background-color:yellow;");
    }

    var result = pattern;
    result = result.replace("{BeginYear}",  parseInt(entry.start_date.split("-")[0], 10));
    result = result.replace("{EndYear}",  parseInt(entry.end_date.split("-")[0], 10));
    result = result.replace("{Title}", entry.title);
    result = result.replace("{Score}", entry.score);
    parent.setInnerText(this.htmlDecode(this.fixUnicode(this.encodeResult(result))));
};


animeRatings.informFailure = function(linkItem) {
    var reason = (linkItem.reason === undefined ? "No results returned." : linkItem.reason);
    var node = linkItem.node;
    var parent = node.parentNode;
    parent = parent.createEntryList().create("li");
    parent.setInnerText(reason);
};


animeRatings.addEntriesToDOM = function(linkItem) {
    var node = linkItem.node;
    var parent = node.parentNode;

    var entries = linkItem.entries;
    entries.sort(function(lhs, rhs) {
        if (lhs.start_date < rhs.start_date) {
            return -1;
        }
        else if (lhs.start_date == rhs.start_date) {
            return 0;
        }
        return 1;
    });

    if (parent.getElementsByTagName("ul").length === 0) {
        parent = parent.createEntryList();
        parent.style.listStyle = "square outside none";
    }

    for (var i = 0; i < entries.length; ++i) {
        try {
            var entry = entries[i];

            // Skip if year doesn't match
            var begin_year = parseInt(entry.start_date.split("-")[0], 10);
            var end_year = parseInt(entry.end_date.split("-")[0], 10);
            var year = this.getYear();
            if (begin_year > year || (end_year !== 0 && end_year < year)) {
                if (entry.title.search("Hunter") !== -1) {
                    this.log(year + " is outside of [" + begin_year + ", " + end_year + "]");
                    this.log("entry.end_date: " + entry.end_date);
                }
                parent.setAttribute("private_year_is_wrong", true);
                continue;
            }

            // Insert title entry
            this.addEntryToDOM(parent.create("li"),
                               entry,
                               "{Title} ({Score})");
        }
        catch (exc) {
            animeRatings.log(exc);
        }
    }

    if (parent.hasAttribute("private_year_is_wrong") &&
        parent.getElementsByTagName("li").length === 0) {
        animeRatings.addMissingStuff(parent, entries);
    }
};


animeRatings.addMissingStuff = function(listElement, entries) {

    if (entries.length === 0) {
        return;
    }

    listElement.style.listStyle = "none";

    var parent = listElement.create("li");
    parent.setInnerText("No MAL title found from " + this.getYear() + ". Closest match:");

    parent = parent.create("ul");
    parent.style.listStyle = "square outside none";

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
            animeRatings.log(exc);
        }
    }

    // Insert title entry
    this.addEntryToDOM(parent.create("li"), closest_entry, "{Title} ({BeginYear}, {Score})");
};


/**
 * Workaround: improves search results
 */
animeRatings.improveTitle = function(title) {

    var result = title;

    // If the title contains a fragment, then only search for the mapped fragment.
    var tmapping = {
        "11eyes: Tsumi to Batsu to Aganai no Shōjo" : "11eyes",
        "A Channel": "A-Channel",
        "Aki Sora" : "Aki-Sora",
        "Ano Hi Mita Hana": "Ano Hi Mita Hana",
        "Doraemon" : "Doraemon",
        "Infinite Stratos" : "Infinite Stratos",
        "Mawaru-Penguindrum" : "Mawaru Penguindrum",
        "Poppy Hill" : "Kokurikozaka Kara",
        "Koishinasai" : "Koi Shinasai",
        "Heaven's Lost Property" : "Heaven's Lost Property"
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


animeRatings.linkNodes = {};
animeRatings.links = animeRatings.getLinks().reverse();

Element.prototype.setInnerText = function(text) {
    this.appendChild(document.createTextNode(text));
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

animeRatings.getNext = function() {
    if (animeRatings.links.length === 0) {
        return;
    }

    var linkNode = animeRatings.links.pop();

    var title = animeRatings.improveTitle(linkNode.title);
    if (animeRatings.linkNodes[title] === undefined) {
        animeRatings.linkNodes[title] = linkNode;
        this.getMALInfo(title, function(linkInfo) {
            linkInfo.node = animeRatings.linkNodes[title];
            if (linkInfo.success === true) {
                animeRatings.addEntriesToDOM(linkInfo);
            }
            else {
                animeRatings.informFailure(linkInfo);
            }
            animeRatings.getNext();
        });
    }
};


//
// Application Entry Point
//
var numSimulReq = 5;
for (var i = 0; i < numSimulReq; ++i) {
    animeRatings.getNext(animeRatings.links);
}


} catch (exc) {
    alert(exc);
}
