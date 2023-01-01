// https://golangbyexample.com/basic-http-server-go/
package main

import (
    "flag"
    "log"
    "fmt"
    "time"
    "strings"
    "io/ioutil"
    "net/http"
)

// input options
var (
    addr_opt = flag.String("listen-address", ":8080", "The address to listen on for HTTP requests.")
)

type Results struct {
    Temp  string
    Wdir  string
    Wind  string
    Gust  string
    Humi  string
    Pres  string
    Prec  string
    Sun   string
    Moon  string
    Lune  string
}

func meyrin() Results {

    resp1, err1 := http.Get("https://meteo.search.ch/meyrin.de.html")
    if err1 != nil {
        print(err1)
    }
    defer resp1.Body.Close()
    body, err := ioutil.ReadAll(resp1.Body)
    if err != nil {
        print(err)
    }

    var res Results

    //wget 'http://www.aviationweather.gov/adds/dataserver_current/httpparam?dataSource=metars&requestType=retrieve&format=csv&stationString=LSGG&hoursBeforeNow=1'

    var payload = string(body)
    pres_pos := strings.Index(payload, `<td title="Luftdruck auf Stationshöhe (QFE)">`)
    temp_pos := strings.Index(payload, `<td title="Lufttemperatur 2 m über Boden">`)
    wind_pos := strings.Index(payload, `<td title="Zehnminutenmittel">`)
    gust_pos := strings.Index(payload, `<td title="Sekundenböe">`)
    humi_pos := strings.Index(payload, `<td title="2 m über Boden">`)
    prec_pos := strings.Index(payload, `<td title="Summe der letzten 10 Minuten">`)

    if pres_pos >= 0 {
        pres_pos += 46
        res.Pres = payload[pres_pos:pres_pos+10]
        res.Pres = res.Pres[ :strings.Index(res.Pres, `<`)]
    }
    if temp_pos >= 0 {
        temp_pos += 43
	res.Temp = payload[temp_pos:temp_pos+10]
        res.Temp = res.Temp[ :strings.Index(res.Temp, `<`)]
    }
    if wind_pos >= 0 {
        wind_pos += 30
        res.Wind = payload[wind_pos:wind_pos+10]
	res.Wind = res.Wind[ :strings.Index(res.Wind, `<`)]
    }
    if gust_pos >= 0 {
        gust_pos += 25
        res.Gust = payload[gust_pos:gust_pos+10]
        res.Gust = res.Gust[ :strings.Index(res.Gust, `<`)]
    }
    if humi_pos >= 0 {
        humi_pos += 28
        res.Humi = payload[humi_pos:humi_pos+10]
        res.Humi = res.Humi[ :strings.Index(res.Humi, `<`)]
    }
    if prec_pos >= 0 {
        prec_pos += 41
        res.Prec = payload[prec_pos:prec_pos+10]
        res.Prec = res.Prec[ :strings.Index(res.Prec, `<`)]
    }
    // qwe
    resp2, err2 := http.Get("https://meteo.search.ch/astro.de.html")
    if err2 != nil {
        print(err2)
    }
    defer resp2.Body.Close()
    body, err = ioutil.ReadAll(resp2.Body)
    if err != nil {
        print(err)
    }

    payload = string(body)
    sunshine_pos := strings.Index(payload, `<tr><td>Sonne</td>`)
    if sunshine_pos >= 0 {
        sunshine_pos += 40
        res.Sun  = payload[sunshine_pos:sunshine_pos+5] + " "
        res.Sun += payload[sunshine_pos+32:sunshine_pos+37]
    }
    moonshine_pos := strings.Index(payload, `<tr><td>Mond</td>`)
    if moonshine_pos >= 0 {
        moonshine_pos += 39
        res.Moon  = payload[moonshine_pos:moonshine_pos+5] + " "
        res.Moon += payload[moonshine_pos+32:moonshine_pos+37]
    }
    lune_pos := strings.Index(payload, `Vollmond`)
    if lune_pos >= 0 {
        res.Lune = payload[lune_pos:lune_pos+33]
        res.Lune = res.Lune[ :strings.Index(res.Lune, `<`)]
    }

    return res
}


func weather(res http.ResponseWriter, req *http.Request) {
//    fmt.Println("Endpoint Hit: weather")
//    fmt.Println(info.Pres)
//    fmt.Println(info.Temp)
//    fmt.Println(info.Wind)
//    fmt.Println(info.Gust)
//    fmt.Println(info.Humi)
//    fmt.Println(info.Prec)
//    fmt.Println(info.Sun)
//    fmt.Println(info.Moon)
//    fmt.Println(info.Lune)

    info := meyrin()
    fmt.Fprintf(res, "Temperature: %v\n", info.Temp)
    fmt.Fprintf(res, "Presure: %v\n", info.Pres)
    fmt.Fprintf(res, "Wind: %v\n", info.Wind)
    fmt.Fprintf(res, "Gusts: %v\n", info.Gust)
    fmt.Fprintf(res, "Humidity: %v\n", info.Humi)
    fmt.Fprintf(res, "Precepitation: %v\n", info.Prec)
    fmt.Fprintf(res, "Sun: %v\n", info.Sun)
    fmt.Fprintf(res, "Moon: %v\n", info.Moon)
    fmt.Fprintf(res, "%v\n", info.Lune)

    now := time.Now()
    loc, _ := time.LoadLocation("Local")
    fmt.Fprintf(res, "Time: %s\n", now.In(loc).Format(time.RFC3339)) //"2006-01-02 15:04:05.999Z")
}

func main() {
    flag.Parse()

    fmt.Printf("Flags: addr_opt=%v",*addr_opt)

    //Create the default mux
    router := http.NewServeMux()

    //Handling the /v1/teachers. The handler is a function here
    router.HandleFunc("/v1/weather", weather)
    //Create the server. 
    s := &http.Server{
        Addr:    *addr_opt,
        Handler: router,
    }
    log.Fatal( s.ListenAndServe() )

//    log.Fatal( http.ListenAndServe(":8080", router) )
}
