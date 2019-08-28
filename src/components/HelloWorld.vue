<template>
  <div class="hello">
    <h1>{{ bleStatus }}</h1>
    <button v-on:click="readValue">データ取得</button>
    <apexchart width="1024" height="500" type="area" :options="chartOptions" :series="series"></apexchart>
  </div>
</template>
<script>
export default {
  name: 'HelloWorld',
  props: {
    msg: String
  },
  data: function(){
    return {
      USER_SERVICE_UUID: '1966e739-de61-4d30-8098-e5e70ffc687f',
      READ_CHARACTERISTIC_UUID: 'E9062E71-9E62-4BC6-B0D3-35CDCD9B027B', // LINE THings Starterのもので大丈夫
      bleConnect: false,
      bleStatus: `デバイスに接続されるのをお待ち下さい。`,
      user: {
        image: '',
        userId: ''
      },
      chartOptions: {
        chart: {
          toolbar: {
            show: false
          }
        },
        xaxis: {
          type: 'datetime',
        },
        dataLabels: {
          enabled: false
        },
      },
      series: [{
            name: "humidity",
            data: []
      }],    
    }
  },
  methods: {
    //BLEが接続できる状態になるまでリトライ
    liffCheckAvailablityAndDo: async function (callbackIfAvailable) {
      try {
        const isAvailable = await this.$liff.bluetooth.getAvailability();
        if (isAvailable) {
          callbackIfAvailable();
        } else {
          // リトライ
          this.bleStatus = `Bluetoothをオンにしてください。`;
            setTimeout(() => this.liffCheckAvailablityAndDo(callbackIfAvailable), 10000);
        }
      } catch (error) {
        alert('Bluetoothをオンにしてください。');
      }
    },
    //サービスとキャラクタリスティックにアクセス
    liffRequestDevice: async function () {
      const device = await this.$liff.bluetooth.requestDevice();
      await device.gatt.connect();
      const service = await device.gatt.getPrimaryService(this.USER_SERVICE_UUID);
      window.readCharacteristic = await service.getCharacteristic(this.READ_CHARACTERISTIC_UUID);
      // alert('connect');
      this.bleConnect = true;
      this.bleStatus = `デバイスに接続しました。`;
    },
    initializeLiff: async function(){
      await this.$liff.initPlugins(['bluetooth']);
      this.liffCheckAvailablityAndDo(() => this.liffRequestDevice());

      //プロフゲット
      const profile = await this.$liff.getProfile();
      this.user.image = profile.pictureUrl;
      this.user.userId = profile.userId;
    },
    readValue: async function(){
      const view = await window.readCharacteristic.readValue();
      const data = new Uint8Array(view.buffer);
//    const data = new Uint8Array(24*7);
      
      const a = new Array();
      var start = new Date().getTime() - 24 * 7 * 3600000;
      for (var i=0;i<data.length;i++){
        a.push([(start + i * 3600000),data[i]]);
//      a.push([(start + i * 3600000), 255 * Math.random()]);
      }
      this.series[0].data = a;
    }
  },
  mounted: function(){
    this.$liff.init(
      () => this.initializeLiff()
    );
  }
}

</script>

<!-- Add "scoped" attribute to limit CSS to this component only -->
<style scoped>
h3 {
  margin: 40px 0 0;
}
ul {
  list-style-type: none;
  padding: 0;
}
li {
  display: inline-block;
  margin: 0 10px;
}
a {
  color: #42b983;
}
</style>
