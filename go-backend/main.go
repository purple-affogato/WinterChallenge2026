package main

import (
	"encoding/json"
	"fmt"
	"log"
	"net/http"
	"os"
	"time"
	"context"
	"github.com/InfluxCommunity/influxdb3-go/v2/influxdb3"
	"github.com/rs/cors"
	"github.com/influxdata/line-protocol/v2/lineprotocol"
)

func helloWorld(w http.ResponseWriter, r *http.Request) {
	w.WriteHeader(http.StatusAccepted)
	fmt.Fprintln(w, "hello world")
}

func receiveTempRhOnce(w http.ResponseWriter, r *http.Request) {
	if r.Method != "POST" || r.Header.Get("Content-Type") != "application/json" {
		http.Error(w, "not expected request", http.StatusBadRequest)
		return
	}
	d := json.NewDecoder(r.Body)
	d.DisallowUnknownFields()
	t := struct {
		TempIntegral *uint8 `json:"temp_integral"`
		TempDecimal  *uint8 `json:"temp_decimal"`
		RhIntegral   *uint8 `json:"rh_integral"`
		RhDecimal    *uint8 `json:"rh_decimal"`
	}{}
	err := d.Decode(&t)
	if err != nil {
		http.Error(w, err.Error(), http.StatusBadRequest)
		return
	}
	if t.TempDecimal == nil || t.TempIntegral == nil || t.RhDecimal == nil || t.RhIntegral == nil {
		http.Error(w, "missing json fields", http.StatusBadRequest)
		return
	}
	if d.More() {
		http.Error(w, "extra data after json", http.StatusBadRequest)
		return
	}
	var temp float32 = float32(*t.TempIntegral) + 0.1*float32(*t.TempDecimal)
	var rh float32 = float32(*t.RhIntegral) + 0.1*float32(*t.RhDecimal)
	url := os.Getenv("INFLUXDB3_HOST")
	token := os.Getenv("INFLUXDB3_AUTH_TOKEN")
	database := os.Getenv("INFLUXDB3_DATABASE")
	client, err := influxdb3.New(influxdb3.ClientConfig{
		Host:     url,
		Token:    token,
		Database: database,
	})
	defer func (client *influxdb3.Client)  {
		err = client.Close()
		if err != nil {
			panic(err)
		}
	}(client)
	point := influxdb3.NewPoint("data",
		map[string]string{
		"place": "dorm",
		},
		map[string]any{
		"temp": temp,
		"rh":  rh,
		},
		time.Now(),
   	)
   	points := []*influxdb3.Point{point}
	err = client.WritePoints(context.Background(), points, influxdb3.WithPrecision(lineprotocol.Second))
	log.Printf("Temp: %f, RH: %f\n", temp, rh)
	if err != nil {
		log.Printf("failed to write to influxdb3\n")
	}
	w.WriteHeader(http.StatusAccepted)
}

func main() {
	mux := &http.ServeMux{}
	mux.HandleFunc("/", helloWorld)
	mux.HandleFunc("/receive-data-once", receiveTempRhOnce)

	corsHandler := cors.New(cors.Options{
		AllowedOrigins:   []string{"*"},
		AllowedMethods:   []string{http.MethodGet, http.MethodPost},
		AllowedHeaders:   []string{"Content-Type", "User-Agent"},
		AllowCredentials: true,
	}).Handler(mux)

	http.ListenAndServe(":8080", corsHandler)
}
